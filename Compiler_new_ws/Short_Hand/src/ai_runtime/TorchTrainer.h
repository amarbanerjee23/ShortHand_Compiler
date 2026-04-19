#ifndef SHORT_TORCH_TRAINER_H
#define SHORT_TORCH_TRAINER_H

#include <string>

class TorchTrainer {
public:
    TorchTrainer() {}
    ~TorchTrainer() {}

    bool trainLinearModel(int epochs, double learning_rate);
    bool save(const std::string &path);
    std::string getLastError() const;

private:
    std::string last_error_;
};

#endif
