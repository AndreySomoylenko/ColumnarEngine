#include "Pipeline.h"
#include "io/BatchScanner.h"
#include "io/CSVWriter.h"
#include "io/ColumnarReader.h"
#include "io/IOScanner.h"
#include "io/VectorScanner.h"
#include <stdexcept>
#include <utility>

Pipeline::Pipeline(std::vector<std::unique_ptr<Operation>> &&operations,
                   ColumnarReader &reader, const Scheme &scheme,
                   const std::string &result_filename)
    : operations_(std::move(operations)), writer_(result_filename),
      scanner_(std::make_unique<IOScanner>(scheme, reader)) {}

Pipeline::Pipeline(std::vector<std::unique_ptr<Operation>> &&operations,
                   ColumnarReader &reader, Scheme &&scheme,
                   const std::string &result_filename)
    : operations_(std::move(operations)), writer_(result_filename),
      scanner_(std::make_unique<IOScanner>(std::move(scheme), reader)) {}

namespace {

StreamingOperation &AsStreaming(Operation &operation) {
    auto *streaming = dynamic_cast<StreamingOperation *>(&operation);
    if (streaming == nullptr) {
        throw std::logic_error("Expected streaming operation");
    }
    return *streaming;
}

BlockingOperation &AsBlocking(Operation &operation) {
    auto *blocking = dynamic_cast<BlockingOperation *>(&operation);
    if (blocking == nullptr) {
        throw std::logic_error("Expected blocking operation");
    }
    return *blocking;
}

} // namespace

void Pipeline::Execute() {
    size_t operation_index = 0;

    while (operation_index < operations_.size()) {
        std::vector<StreamingOperation *> streaming_operations;
        while (operation_index < operations_.size() &&
               operations_[operation_index]->GetType() ==
                   OperationType::NonBlocking) {
            streaming_operations.push_back(
                &AsStreaming(*operations_[operation_index]));
            ++operation_index;
        }

        if (operation_index == operations_.size()) {
            while (!scanner_->IsEnd()) {
                Batch batch = scanner_->ReadNext();
                for (auto *operation : streaming_operations) {
                    operation->Execute(batch);
                }
                Write(batch);
            }
            writer_.Flush();
            return;
        }

        BlockingOperation &blocking =
            AsBlocking(*operations_[operation_index]);
        while (!scanner_->IsEnd()) {
            Batch batch = scanner_->ReadNext();
            for (auto *operation : streaming_operations) {
                operation->Execute(batch);
            }
            blocking.Process(batch);
        }

        scanner_ =
            std::make_unique<VectorScanner>(std::move(blocking).Finalize());
        ++operation_index;
    }

    while (!scanner_->IsEnd()) {
        Write(scanner_->ReadNext());
    }
    writer_.Flush();
}

void Pipeline::Write(const Batch &batch) {
    for (size_t row = 0; row < batch.VerticalSize(); ++row) {
        if (batch.IsRowEnabled(row)) {
            writer_.WriteRow(batch.GetRow(row));
        }
    }
}
