#include "TorchTrainer.h"

#ifdef USE_LIBTORCH
#include <torch/torch.h>
#endif

bool TorchTrainer::trainLinearModel(int epochs, double learning_rate) {
#ifndef USE_LIBTORCH
    (void)epochs;
    (void)learning_rate;
    this->last_error_ = "LibTorch support not enabled. Rebuild with LIBTORCH_ROOT set.";
    return false;
#else
    try {
        torch::nn::Linear linear(1, 1);
        torch::optim::SGD optimizer(linear->parameters(), torch::optim::SGDOptions(learning_rate));

        torch::Tensor x = torch::tensor({{-2.0f}, {-1.0f}, {0.0f}, {1.0f}, {2.0f}});
        torch::Tensor y = 2.0f * x + 1.0f;

        for (int i = 0; i < epochs; ++i) {
            optimizer.zero_grad();
            torch::Tensor pred = linear->forward(x);
            torch::Tensor loss = torch::mse_loss(pred, y);
            loss.backward();
            optimizer.step();
        }
        this->last_error_.clear();
        return true;
    } catch (const std::exception &e) {
        this->last_error_ = e.what();
        return false;
    }
#endif
}

bool TorchTrainer::save(const std::string &path) {
#ifndef USE_LIBTORCH
    (void)path;
    this->last_error_ = "LibTorch support not enabled. Rebuild with LIBTORCH_ROOT set.";
    return false;
#else
    try {
        torch::Tensor model_meta = torch::tensor({1.0f});
        torch::save(model_meta, path);
        this->last_error_.clear();
        return true;
    } catch (const std::exception &e) {
        this->last_error_ = e.what();
        return false;
    }
#endif
}

std::string TorchTrainer::getLastError() const {
    return this->last_error_;
}
