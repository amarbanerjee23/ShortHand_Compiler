#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include "./ai_runtime/AI_Runtime.h"

static std::vector<float> parseFloatList(const std::string &text) {
    std::vector<float> values;
    std::stringstream ss(text);
    std::string token;
    while (std::getline(ss, token, ',')) {
        values.push_back(std::stof(token));
    }
    return values;
}

static std::vector<int64_t> parseIntList(const std::string &text) {
    std::vector<int64_t> values;
    std::stringstream ss(text);
    std::string token;
    while (std::getline(ss, token, ',')) {
        values.push_back(std::stoll(token));
    }
    return values;
}

int main(int argc, char **argv) {
    if (argc < 4) {
        std::cerr << "Usage: short_ai_app <model.onnx> <shape_csv> <input_csv>\n";
        return 1;
    }

    std::string model = argv[1];
    std::vector<int64_t> shape = parseIntList(argv[2]);
    std::vector<float> input = parseFloatList(argv[3]);

    AI_Runtime runtime;
    if (!runtime.loadModel(model)) {
        std::cerr << runtime.getLastError() << "\n";
        return 1;
    }

    TensorData tensor{shape, input};
    std::vector<float> output;
    if (!runtime.run(tensor, output)) {
        std::cerr << runtime.getLastError() << "\n";
        return 1;
    }

    std::cout << "Output:";
    for (float v : output) {
        std::cout << " " << v;
    }
    std::cout << "\n";
    return 0;
}
