#pragma once

#include "core/Pipeline.h"
#include "data_structures/Scheme.h"
#include "io/ColumnarReader.h"

class Engine {
  public:
    Engine(const Filename &data, const Filename &scheme,
           const Filename &columnar = "test.hub");
    explicit Engine(const Filename &columnar);

    void TakeAll(const Filename &result_name);
    void Execute(Pipeline &pipeline);

    void ExecuteClickBench();

    void Make01Querry();
    void Make02Querry();
    void Make03Querry();
    void Make04Querry();
    void Make05Querry();
    void Make06Querry();
    void Make07Querry();
    void Make08Querry();
    void Make09Querry();
    void Make10Querry();
    void Make11Querry();
    void Make12Querry();
    void Make13Querry();
    void Make14Querry();
    void Make15Querry();
    void Make16Querry();
    void Make17Querry();
    void Make18Querry();
    void Make19Querry();
    void Make20Querry();
    void Make21Querry();
    void Make22Querry();
    void Make23Querry();
    void Make24Querry();
    void Make25Querry();
    void Make26Querry();
    void Make27Querry();
    void Make28Querry();
    void Make29Querry();
    void Make30Querry();
    void Make31Querry();
    void Make32Querry();
    void Make33Querry();
    void Make34Querry();
    void Make35Querry();
    void Make36Querry();
    void Make37Querry();
    void Make38Querry();
    void Make39Querry();
    void Make40Querry();
    void Make41Querry();
    void Make42Querry();
    void Make43Querry();

  private:
    ColumnarReader reader_;
};
