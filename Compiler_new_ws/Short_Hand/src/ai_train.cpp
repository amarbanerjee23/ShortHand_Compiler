#include <iostream>
#include <string>
#include "./ai_runtime/TorchTrainer.h"

int main(int argc, char **argv) {
    if (argc < 4) {
        std::cerr << "Usage: short_ai_train <epochs> <learning_rate> <output_model.pt>\n";
        return 1;
    }

    int epochs = std::stoi(argv[1]);
    double learning_rate = std::stod(argv[2]);
    std::string model_path = argv[3];

    TorchTrainer trainer;
    if (!trainer.trainLinearModel(epochs, learning_rate)) {
        std::cerr << trainer.getLastError() << "\n";
        return 1;
    }
    if (!trainer.save(model_path)) {
        std::cerr << trainer.getLastError() << "\n";
        return 1;
    }

    std::cout << "Training complete. Saved model to " << model_path << "\n";
    return 0;
}
