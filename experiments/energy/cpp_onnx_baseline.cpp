// Independent native C++ / ONNX Runtime CPU baseline for PR104 energy experiments.
// This file deliberately does not use ShortHand runtime code.
#include <onnxruntime_cxx_api.h>

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <numeric>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

struct Dataset {
  // Raw Optdigits pixels in [0, 16]. Normalization is intentionally repeated
  // inside every classify() call so preprocessing remains inside the same
  // inference boundary used by the native/Python controls.
  std::vector<float> values;
  std::vector<int64_t> labels;
};

struct Trial {
  double start_unix_seconds;
  double end_unix_seconds;
  double elapsed_ms;
  int64_t completed;
};

Dataset read_csv(const std::string &path) {
  std::ifstream in(path);
  if (!in) throw std::runtime_error("cannot open dataset");
  Dataset out;
  std::string line;
  while (std::getline(in, line)) {
    if (line.empty()) continue;
    std::stringstream row(line);
    std::string token;
    std::vector<double> fields;
    while (std::getline(row, token, ',')) fields.push_back(std::stod(token));
    if (fields.size() != 65) throw std::runtime_error("invalid dataset width");
    for (size_t i = 0; i < 64; ++i) {
      if (!std::isfinite(fields[i]) || fields[i] < 0.0 || fields[i] > 16.0)
        throw std::runtime_error("invalid feature value");
      out.values.push_back(static_cast<float>(fields[i]));
    }
    const auto label = static_cast<int64_t>(fields[64]);
    if (fields[64] != static_cast<double>(label) || label < 0 || label > 9)
      throw std::runtime_error("invalid label");
    out.labels.push_back(label);
  }
  if (out.labels.size() != 1797 || out.values.size() != 1797 * 64)
    throw std::runtime_error("unexpected Optdigits test shape");
  return out;
}

std::string input_name(Ort::Session &session, Ort::AllocatorWithDefaultOptions &allocator) {
#if ORT_API_VERSION >= 12
  auto p = session.GetInputNameAllocated(0, allocator);
  return p ? p.get() : "";
#else
  char *p = session.GetInputName(0, allocator);
  std::string name = p ? p : "";
  allocator.Free(p);
  return name;
#endif
}

std::string output_name(Ort::Session &session, Ort::AllocatorWithDefaultOptions &allocator) {
#if ORT_API_VERSION >= 12
  auto p = session.GetOutputNameAllocated(0, allocator);
  return p ? p.get() : "";
#else
  char *p = session.GetOutputName(0, allocator);
  std::string name = p ? p : "";
  allocator.Free(p);
  return name;
#endif
}

struct Classification {
  std::vector<float> scores;
  std::vector<int64_t> predictions;
  std::vector<int64_t> top_k;
};

Classification classify(Ort::Session &session,
                        const std::string &in_name,
                        const std::string &out_name,
                        const Dataset &data,
                        size_t batch) {
  Classification out;
  out.scores.reserve(data.labels.size() * 10);
  out.predictions.reserve(data.labels.size());
  out.top_k.reserve(data.labels.size() * 3);
  Ort::MemoryInfo memory = Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeDefault);
  const char *inputs[] = {in_name.c_str()};
  const char *outputs[] = {out_name.c_str()};
  for (size_t offset = 0; offset < data.labels.size(); offset += batch) {
    const size_t count = std::min(batch, data.labels.size() - offset);
    std::vector<float> buffer(batch * 64, 0.0f);
    for (size_t i = 0; i < count * 64; ++i) {
      const float raw = data.values[offset * 64 + i];
      if (!std::isfinite(raw) || raw < 0.0f || raw > 16.0f)
        throw std::runtime_error("input outside declared range");
      buffer[i] = raw / 16.0f;
      if (!std::isfinite(buffer[i]))
        throw std::runtime_error("preprocessing produced nonfinite value");
    }
    std::vector<int64_t> shape = {static_cast<int64_t>(batch), 64};
    auto tensor = Ort::Value::CreateTensor<float>(
        memory, buffer.data(), buffer.size(), shape.data(), shape.size());
    auto result = session.Run(Ort::RunOptions{nullptr}, inputs, &tensor, 1, outputs, 1);
    if (result.size() != 1 || !result[0].IsTensor())
      throw std::runtime_error("invalid ONNX output");
    auto info = result[0].GetTensorTypeAndShapeInfo();
    const std::vector<int64_t> expected_shape = {static_cast<int64_t>(batch), 10};
    if (info.GetElementType() != ONNX_TENSOR_ELEMENT_DATA_TYPE_FLOAT ||
        info.GetShape() != expected_shape || info.GetElementCount() != batch * 10)
      throw std::runtime_error("unexpected ONNX output shape");
    const float *scores = result[0].GetTensorData<float>();
    for (size_t i = 0; i < batch * 10; ++i)
      if (!std::isfinite(scores[i])) throw std::runtime_error("nonfinite ONNX output");
    out.scores.insert(out.scores.end(), scores, scores + count * 10);
    for (size_t row = 0; row < count; ++row) {
      std::array<int64_t, 10> order{};
      std::iota(order.begin(), order.end(), int64_t{0});
      std::partial_sort(order.begin(), order.begin() + 3, order.end(),
                        [&](int64_t a, int64_t b) {
                          const float x = scores[row * 10 + static_cast<size_t>(a)];
                          const float y = scores[row * 10 + static_cast<size_t>(b)];
                          return x == y ? a < b : x > y;
                        });
      out.predictions.push_back(order.front());
      out.top_k.insert(out.top_k.end(), order.begin(), order.begin() + 3);
    }
  }
  return out;
}

int positive(const char *value, const char *name, int maximum = 10000) {
  char *end = nullptr;
  const long parsed = std::strtol(value, &end, 10);
  if (!value[0] || !end || *end != '\0' || parsed < 1 || parsed > maximum)
    throw std::runtime_error(std::string("invalid ") + name);
  return static_cast<int>(parsed);
}

double unix_seconds(std::chrono::system_clock::time_point value) {
  return std::chrono::duration<double>(value.time_since_epoch()).count();
}

void write_trials(const std::string &path, const std::vector<Trial> &trials) {
  if (path.empty()) return;
  std::ofstream out(path, std::ios::trunc);
  if (!out) throw std::runtime_error("cannot create trial report");
  out << "start_unix_seconds,end_unix_seconds,elapsed_ms,completed\n";
  out << std::setprecision(17);
  for (const auto &trial : trials)
    out << trial.start_unix_seconds << ',' << trial.end_unix_seconds << ','
        << trial.elapsed_ms << ',' << trial.completed << '\n';
  out.flush();
  if (!out) throw std::runtime_error("failed to finalize trial report");
}

}  // namespace

int main(int argc, char **argv) {
  try {
    if (argc == 2 && std::string(argv[1]) == "--version") {
      std::cout << OrtGetApiBase()->GetVersionString() << '\n';
      return 0;
    }
    if (argc != 8 && argc != 9) {
      std::cerr << "usage: cpp_onnx_baseline MODEL DATASET BATCH THREADS WARMUPS REPETITIONS TRIALS [TRIAL_REPORT]\n";
      return 2;
    }
#ifdef _WIN32
    _putenv_s("ORT_DISABLE_TELEMETRY", "1");
#else
    setenv("ORT_DISABLE_TELEMETRY", "1", 1);
#endif
    const std::string model = argv[1];
    const std::string dataset_path = argv[2];
    const int batch = positive(argv[3], "batch", 1024);
    const int threads = positive(argv[4], "threads", 256);
    const int warmups = positive(argv[5], "warmups", 100);
    const int repetitions = positive(argv[6], "repetitions");
    const int trials = positive(argv[7], "trials", 100);
    const std::string trial_report = argc == 9 ? argv[8] : "";

    Dataset data = read_csv(dataset_path);
    Ort::Env env(ORT_LOGGING_LEVEL_WARNING, "shorthand_independent_cpp_baseline");
    env.DisableTelemetryEvents();
    Ort::SessionOptions options;
    options.SetIntraOpNumThreads(threads);
    options.SetInterOpNumThreads(1);
    options.SetExecutionMode(ExecutionMode::ORT_SEQUENTIAL);
    options.SetGraphOptimizationLevel(GraphOptimizationLevel::ORT_ENABLE_BASIC);
    options.AddConfigEntry("session.intra_op.allow_spinning", "0");
    options.AddConfigEntry("session.inter_op.allow_spinning", "0");
    Ort::Session session(env, model.c_str(), options);
    if (session.GetInputCount() != 1 || session.GetOutputCount() != 1)
      throw std::runtime_error("expected one input and one output");
    Ort::AllocatorWithDefaultOptions allocator;
    const std::string in_name = input_name(session, allocator);
    const std::string out_name = output_name(session, allocator);
    if (in_name.empty() || out_name.empty()) throw std::runtime_error("missing ONNX names");

    for (int n = 0; n < warmups; ++n)
      (void)classify(session, in_name, out_name, data, static_cast<size_t>(batch));

    Classification reference;
    Classification current;
    bool have_reference = false;
    std::vector<Trial> trial_records;
    trial_records.reserve(static_cast<size_t>(trials));
    for (int trial = 0; trial < trials; ++trial) {
      const auto wall_start = std::chrono::system_clock::now();
      const auto steady_start = std::chrono::steady_clock::now();
      int64_t completed = 0;
      for (int rep = 0; rep < repetitions; ++rep) {
        current = classify(session, in_name, out_name, data, static_cast<size_t>(batch));
        if (!have_reference) {
          reference = current;
          have_reference = true;
        } else {
          if (current.predictions != reference.predictions ||
              current.top_k != reference.top_k ||
              current.scores.size() != reference.scores.size())
            throw std::runtime_error("C++ ONNX prediction regression");
          for (size_t i = 0; i < current.scores.size(); ++i) {
            const double a = current.scores[i];
            const double b = reference.scores[i];
            if (std::abs(a - b) > 1e-5 + 1e-4 * std::abs(b))
              throw std::runtime_error("C++ ONNX numerical regression");
          }
        }
        completed += static_cast<int64_t>(current.predictions.size());
      }
      const auto steady_end = std::chrono::steady_clock::now();
      const auto wall_end = std::chrono::system_clock::now();
      const double elapsed_ms =
          std::chrono::duration<double, std::milli>(steady_end - steady_start).count();
      trial_records.push_back(
          {unix_seconds(wall_start), unix_seconds(wall_end), elapsed_ms, completed});
    }

    size_t correct = 0;
    for (size_t i = 0; i < current.predictions.size(); ++i)
      if (current.predictions[i] == data.labels[i]) ++correct;
    if (static_cast<double>(correct) / data.labels.size() < 0.85)
      throw std::runtime_error("accuracy below predeclared threshold");

    const int64_t prediction_sum =
        std::accumulate(current.predictions.begin(), current.predictions.end(), int64_t{0});
    const int64_t checksum = prediction_sum * repetitions * trials;
    write_trials(trial_report, trial_records);
    for (auto value : current.predictions) std::cout << value << '\n';
    std::cout << checksum << '\n';
    return 0;
  } catch (const Ort::Exception &ex) {
    std::cerr << "onnxruntime exception: " << ex.what() << '\n';
  } catch (const std::exception &ex) {
    std::cerr << ex.what() << '\n';
  }
  return 1;
}
