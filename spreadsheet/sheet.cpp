#include "sheet.h"

#include "cell.h"
#include "common.h"

#include <algorithm>
#include <functional>
#include <iostream>
#include <optional>

using namespace std::literals;

Sheet::~Sheet() {
}

void Sheet::SetCell(Position pos, std::string text) {

    if (!pos.IsValid()) {
        throw InvalidPositionException("SetCell ERROR: InvalidPosition.");
    }

    if (ptr_table_.size() <= size_t(pos.row)) {
        ptr_table_.resize(pos.row + 1);
    }

    if (ptr_table_.at(pos.row).size() <= size_t(pos.col)) {
        ptr_table_[pos.row].resize(pos.col + 1);
    }

    // update minimal print area
    if (pos.col + 1 > max_width_) {
        max_width_  = pos.col + 1;
    }
    if (pos.row + 1 > max_height_) {
        max_height_ = pos.row + 1;
    }

    if (!text.empty() && text[0] == '=' && text != "=") {
        auto formula = ParseFormula(text.substr(1));
        auto referenced = formula->GetReferencedCells();
        bool is_cycle = FindCyclicDependencies(referenced, pos);

        if (is_cycle) {
            throw CircularDependencyException("Sheet ERROR: Formula Circular Dependency");
        }
    }

    ptr_table_[pos.row][pos.col] = std::make_unique<Cell>(*this);
    auto ptr = dynamic_cast<Cell*>(GetCell(pos));
    ptr->Set(text);

    for (auto i : ptr->GetReferencedCells()) {
        auto cell = GetCell(i);
        if (cell == nullptr) {
            SetCell(i, "");
        }
    }

}

const CellInterface* Sheet::GetCell(Position pos) const {

    if (!pos.IsValid()) {
        throw InvalidPositionException("GetCell ERROR: InvalidPosition.");
    }

    if (ptr_table_.size() > size_t(pos.row) && ptr_table_.at(pos.row).size() > size_t(pos.col)) {
        return ptr_table_[pos.row][pos.col].get();
    }

    return nullptr;
}

CellInterface* Sheet::GetCell(Position pos) {

    if (!pos.IsValid()) {
        throw InvalidPositionException("GetCell ERROR: InvalidPosition.");
    }

    if (ptr_table_.size() > size_t(pos.row) && ptr_table_.at(pos.row).size() > size_t(pos.col)) {
        return ptr_table_[pos.row][pos.col].get();
    }

    return nullptr;
}

void Sheet::ClearCell(Position pos) {

    if (!pos.IsValid()) {
        throw InvalidPositionException("ClearCell ERROR: InvalidPosition.");
    }

    if (pos.IsValid() && ptr_table_.size() > size_t(pos.row) && ptr_table_.at(pos.row).size() > size_t(pos.col)) {
        ptr_table_[pos.row][pos.col].reset();
    }

    FindAndDecreaseMaxHeightAndWidth();
}

Size Sheet::GetPrintableSize() const {
    return Size{max_height_, max_width_};
}

void Sheet::PrintValues(std::ostream& output) const {

    if (ptr_table_.empty()) {
        return;
    }

    for (auto row = 0; row < max_height_; ++row) {
        bool is_first = true;
        for (auto col = 0; col < max_width_; ++col) {
            if (!is_first) {
                output << '\t';
            }
            if (ptr_table_[row].size() > size_t(col) && ptr_table_[row][col] != nullptr) {
                auto val = ptr_table_[row][col]->GetValue();
                if (std::holds_alternative<double>(val)) {
                    output << std::get<double>(val);
                } else if (std::holds_alternative<std::string>(val)) {
                    output << std::get<std::string>(val);
                } else if (std::holds_alternative<FormulaError>(val)) {
                    output << std::get<FormulaError>(val);
                }
            }
            is_first = false;
        }
        output << '\n';
    }
}

void Sheet::PrintTexts(std::ostream& output) const {

    if (ptr_table_.empty()) {
        return;
    }

    for (auto row = 0; row < max_height_; ++row) {
        bool is_first = true;
        for (auto col = 0; col < max_width_; ++col) {
            if (!is_first) {
                output << '\t';
            }
            if (ptr_table_[row].size() > size_t(col) && ptr_table_[row][col] != nullptr) {
                output << ptr_table_[row][col]->GetText();
            }
            is_first = false;
        }
        output << '\n';
    }
}

void Sheet::FindAndDecreaseMaxHeightAndWidth() {
    int height = 0;
    int width = 0;
    for (auto row = 0; row < int(ptr_table_.size()); ++row) {
        int line_max = 0;
        for (auto col = int(ptr_table_[row].size()); col > 0; --col) {
            if (ptr_table_[row][col - 1] != nullptr) {
                line_max = col;
                height =  row + 1 > height ? row + 1 : height;
                width =  col > width ? col: width;
                break;
            }
        }
        ptr_table_[row].resize(line_max);
    }
    ptr_table_.resize(height + 1);

    max_width_ = width;
    max_height_ = height;
}

std::unique_ptr<SheetInterface> CreateSheet() {
    return std::make_unique<Sheet>();
}

bool Sheet::FindCyclicDependencies(const std::vector<Position>& previous_cells, Position pos) const {

    if (previous_cells.empty()) {
        return false;
    }

    if (std::find(previous_cells.begin(), previous_cells.end(), pos) != previous_cells.end()) {
        return true;
    }

    for (const auto& i : previous_cells) {
        auto ptr = GetCell(i);
        if (ptr != nullptr) {
            bool result = FindCyclicDependencies( ptr->GetReferencedCells(), pos);
            if (result) {
                return true;
            }
        }
    }

    return false;
}