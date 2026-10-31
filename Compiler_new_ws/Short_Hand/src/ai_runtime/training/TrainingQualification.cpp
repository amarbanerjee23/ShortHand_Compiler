#include "TrainingQualification.h"
#include "PersistentWorkers.h"
#include "../ExecutionPlan.h"
#include <algorithm>
#include <cmath>
#include <exception>
#include <stdexcept>
#include <thread>
#include <memory>
namespace shorthand::ai {
namespace {
std::uint32_t nextRandom(std::uint32_t &s) { s^=s<<13; s^=s>>17; s^=s<<5; return s; }
struct Example { std::array<float,64> image{}; unsigned label=0; };
std::vector<Example> dataset(unsigned count,std::uint32_t seed) {
    std::vector<Example> result(count); auto s=seed?seed:1;
    for (unsigned n=0;n<count;++n) {
        auto &e=result[n]; e.label=n%2;
        for (unsigned y=0;y<8;++y) for (unsigned x=0;x<8;++x)
            e.image[y*8+x]=(((e.label?y:x)%2)?0.8f:-0.8f)+(float(nextRandom(s)%1000)/1000.0f-0.5f)*0.2f;
    }
    return result;
}
template<unsigned channels,unsigned side,unsigned filters>
std::array<float,filters*(side-2)*(side-2)> convolution(const std::array<float,channels*side*side> &in,
                               const std::vector<float> &w,std::size_t offset) {
    constexpr unsigned width=side-2; std::array<float,filters*width*width> out{}; auto bias=offset+filters*channels*9;
    for (unsigned f=0;f<filters;++f) for (unsigned y=0;y<width;++y) for (unsigned x=0;x<width;++x) {
        float sum=w[bias+f];
        for (unsigned c=0;c<channels;++c) for (unsigned ky=0;ky<3;++ky) for (unsigned kx=0;kx<3;++kx)
            sum+=in[(c*side+y+ky)*side+x+kx]*w[offset+((f*channels+c)*3+ky)*3+kx];
        out[(f*width+y)*width+x]=std::max(0.0f,sum);
    }
    return out;
}
template<unsigned channels,unsigned side,unsigned filters>
std::array<float,channels*side*side> backward(const std::array<float,channels*side*side> &in,
    const std::array<float,filters*(side-2)*(side-2)> &out,const std::array<float,filters*(side-2)*(side-2)> &derivative,
    const std::vector<float> &w,std::size_t offset,std::vector<float> &gradient) {
    constexpr unsigned width=side-2; auto bias=offset+filters*channels*9; std::array<float,channels*side*side> input_derivative{};
    for (unsigned f=0;f<filters;++f) for (unsigned y=0;y<width;++y) for (unsigned x=0;x<width;++x) {
        auto o=(f*width+y)*width+x; float d=out[o]>0?derivative[o]:0; gradient[bias+f]+=d;
        for (unsigned c=0;c<channels;++c) for (unsigned ky=0;ky<3;++ky) for (unsigned kx=0;kx<3;++kx) {
            auto wi=offset+((f*channels+c)*3+ky)*3+kx;
            auto ii=(c*side+y+ky)*side+x+kx;
            gradient[wi]+=d*in[ii]; input_derivative[ii]+=d*w[wi];
        }
    }
    return input_derivative;
}
std::pair<double,double> evaluate(const ReferenceCNN &m,const std::vector<Example> &data) {
    double loss=0; unsigned correct=0;
    for (const auto &e:data) { unsigned pred=0; loss+=m.lossAndGradient(e.image,e.label,nullptr,&pred); correct+=pred==e.label; }
    return {loss/data.size(),double(correct)/data.size()};
}
}
ReferenceCNN::ReferenceCNN(std::uint32_t seed):parameters(parameter_count) {
    auto s=seed?seed:1; for (auto &p:parameters) p=(float(nextRandom(s)%10000)/10000.0f-0.5f)*0.4f;
    for (unsigned i=36;i<40;++i) parameters[i]=0.05f;
    for (unsigned i=328;i<336;++i) parameters[i]=0.05f;
    parameters[352]=parameters[353]=0;
}
float ReferenceCNN::lossAndGradient(const std::array<float,64> &image,unsigned label,std::vector<float> *gradient,unsigned *prediction) const {
    if (label>1 || parameters.size()!=parameter_count) throw std::runtime_error("invalid_cnn_input");
    for (float v:image) if (!std::isfinite(v)) throw std::runtime_error("nonfinite_cnn_input");
    const auto &in=image;
    auto a=convolution<1,8,4>(in,parameters,0);
    auto b=convolution<4,6,8>(a,parameters,40);
    std::array<float,8> pool{};
    for (unsigned c=0;c<8;++c) for (unsigned i=0;i<16;++i) pool[c]+=b[c*16+i]/16.0f;
    std::array<float,2> logits{parameters[352],parameters[353]};
    for (unsigned o=0;o<2;++o) for (unsigned c=0;c<8;++c) logits[o]+=pool[c]*parameters[336+o*8+c];
    if (prediction) *prediction=logits[1]>logits[0]?1:0;
    float maximum=std::max(logits[0],logits[1]), e0=std::exp(logits[0]-maximum), e1=std::exp(logits[1]-maximum);
    float loss=maximum+std::log(e0+e1)-logits[label];
    if (!std::isfinite(loss)) throw std::runtime_error("nonfinite_training_loss");
    if (!gradient) return loss;
    gradient->assign(parameter_count,0); std::array<float,2> d{e0/(e0+e1),e1/(e0+e1)}; d[label]-=1;
    std::array<float,8> pd{};
    std::array<float,128> bd{};
    for (unsigned o=0;o<2;++o) {
        (*gradient)[352+o]=d[o];
        for (unsigned c=0;c<8;++c) { (*gradient)[336+o*8+c]=d[o]*pool[c]; pd[c]+=d[o]*parameters[336+o*8+c]; }
    }
    for (unsigned c=0;c<8;++c) for (unsigned i=0;i<16;++i) bd[c*16+i]=pd[c]/16.0f;
    auto ad=backward<4,6,8>(a,b,bd,parameters,40,*gradient);
    (void)backward<1,8,4>(in,a,ad,parameters,0,*gradient); return loss;
}
TrainingOutcome trainCpuQualification(const TrainingConfiguration &c,energy::EnergyCollector &collector) {
    if (!c.epochs || c.epochs>100 || c.samples<8 || c.samples>4096 || c.validation_samples<2 || c.validation_samples>4096 ||
        !c.batch_size || c.batch_size>128 || c.samples%c.batch_size || !c.threads || c.threads>availableCpuThreads() ||
        c.threads>c.batch_size || !c.seed || !std::isfinite(c.learning_rate) || c.learning_rate<=0 || c.learning_rate>1 ||
        !std::isfinite(c.target_accuracy) || c.target_accuracy<0 || c.target_accuracy>1) throw std::runtime_error("invalid_training_configuration");
    TrainingOutcome out; auto begin=collector.begin(); auto clock=std::chrono::steady_clock::now();
    try {
        auto data=dataset(c.samples,c.seed^0x12345678U), validation=dataset(c.validation_samples,c.seed^0x87654321U);
        ReferenceCNN model(c.seed); out.initial_loss=evaluate(model,validation).first;
        std::vector<std::vector<float>> gradients(c.batch_size,std::vector<float>(ReferenceCNN::parameter_count));
        std::vector<float> losses(c.batch_size);
        unsigned batch_start=0;
        auto worker=[&](unsigned t) {
            for(unsigned i=t;i<c.batch_size;i+=c.threads)
                losses[i]=model.lossAndGradient(data[batch_start+i].image,data[batch_start+i].label,&gradients[i]);
        };
        std::unique_ptr<training_detail::PersistentWorkers> workers;
        if(c.threads>1) workers=std::make_unique<training_detail::PersistentWorkers>(c.threads,worker);
        out.epochs.reserve(c.epochs);
        out.steps.reserve(static_cast<std::size_t>(c.epochs)*(c.samples/c.batch_size));
        for (unsigned epoch=0;epoch<c.epochs;++epoch) {
            auto epoch_start=collector.begin(); double loss=0;
            for (unsigned start=0;start<c.samples;start+=c.batch_size) {
                auto step_start=collector.begin(); batch_start=start;
                if(workers) workers->run(); else worker(0);
                // Fixed-order FP32 reduction keeps optimizer semantics identical.
                for (std::size_t p=0;p<model.parameters.size();++p) {
                    float sum=0; for (unsigned i=0;i<c.batch_size;++i) sum+=gradients[i][p];
                    model.parameters[p]-=c.learning_rate*(sum/static_cast<float>(c.batch_size));
                }
                for (float v:losses) loss+=v;
                out.steps.push_back(collector.end(step_start,c.batch_size)); out.samples_processed+=c.batch_size; ++out.optimizer_steps;
            }
            auto quality=evaluate(model,validation); TrainingEpoch e; e.index=epoch+1; e.samples_processed=c.samples;
            e.loss=loss/c.samples; e.validation_accuracy=quality.second; e.energy=collector.end(epoch_start,c.samples); out.epochs.push_back(e);
            out.final_loss=quality.first; out.validation_accuracy=quality.second;
        }
        out.parameters=std::move(model.parameters); out.success=out.validation_accuracy>=c.target_accuracy && out.final_loss<out.initial_loss;
        out.reason=out.success?"training_quality_satisfied":"training_quality_threshold_failed";
    } catch (const std::exception &e) { out.reason=e.what(); }
    out.total_energy=collector.end(begin,std::max<std::uint64_t>(1,out.samples_processed));
    if (!out.samples_processed) out.total_energy.functional_units=0;
    out.wall_seconds=std::chrono::duration<double>(std::chrono::steady_clock::now()-clock).count(); return out;
}
}
