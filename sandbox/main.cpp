#include "core/Engine.h"

#include <cstdlib>
#include <exception>
#include <iostream>
#include <string>

namespace {

const char* NeedEnv(const char* name) {
    const char* value = std::getenv(name);
    if (!value || std::string(value).empty()) {
        throw std::runtime_error(std::string("Need env var: ") + name);
    }
    return value;
}

void RunQuery(int q, Engine& engine) {
    switch (q) {
    case 0: engine.Make01Querry(); break;
    case 1: engine.Make02Querry(); break;
    case 2: engine.Make03Querry(); break;
    case 3: engine.Make04Querry(); break;
    case 4: engine.Make05Querry(); break;
    case 5: engine.Make06Querry(); break;
    case 6: engine.Make07Querry(); break;
    case 7: engine.Make08Querry(); break;
    case 8: engine.Make09Querry(); break;
    case 9: engine.Make10Querry(); break;
    case 10: engine.Make11Querry(); break;
    case 11: engine.Make12Querry(); break;
    case 12: engine.Make13Querry(); break;
    case 13: engine.Make14Querry(); break;
    case 14: engine.Make15Querry(); break;
    case 15: engine.Make16Querry(); break;
    case 16: engine.Make17Querry(); break;
    case 17: engine.Make18Querry(); break;
    case 18: engine.Make19Querry(); break;
    case 19: engine.Make20Querry(); break;
    case 20: engine.Make21Querry(); break;
    case 21: engine.Make22Querry(); break;
    case 22: engine.Make23Querry(); break;
    case 23: engine.Make24Querry(); break;
    case 24: engine.Make25Querry(); break;
    case 25: engine.Make26Querry(); break;
    case 26: engine.Make27Querry(); break;
    case 27: engine.Make28Querry(); break;
    case 28: engine.Make29Querry(); break;
    case 29: engine.Make30Querry(); break;
    case 30: engine.Make31Querry(); break;
    case 31: engine.Make32Querry(); break;
    case 32: engine.Make33Querry(); break;
    case 33: engine.Make34Querry(); break;
    case 34: engine.Make35Querry(); break;
    case 35: engine.Make36Querry(); break;
    case 36: engine.Make37Querry(); break;
    case 37: engine.Make38Querry(); break;
    case 38: engine.Make39Querry(); break;
    case 39: engine.Make40Querry(); break;
    case 40: engine.Make41Querry(); break;
    case 41: engine.Make42Querry(); break;
    case 42: engine.Make43Querry(); break;
    default: throw std::runtime_error("Query must be from 0 to 42");
    }
}

} // namespace

int main(int argc, char** argv) {
    try {
        if (argc < 2) {
            std::cerr << "usage: sandbox_app convert | run <query>\n";
            return 2;
        }

        std::string mode = argv[1];

        if (mode == "convert") {
            const char* input_csv = NeedEnv("INPUT_CSV");
            const char* scheme = NeedEnv("SCHEME");
            const char* columnar = NeedEnv("COLUMNAR");

            Engine engine(input_csv, scheme, columnar);
            return 0;
        }

        if (mode == "run") {
            if (argc != 3) {
                std::cerr << "usage: sandbox_app run <query>\n";
                return 2;
            }

            const char* columnar = NeedEnv("COLUMNAR");
            int query = std::stoi(argv[2]);

            Engine engine(columnar);
            RunQuery(query, engine);
            return 0;
        }

        std::cerr << "unknown mode: " << mode << '\n';
        return 2;
    } catch (const std::exception& e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}