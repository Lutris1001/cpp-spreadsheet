#include "sheet.h"

#include "cell.h"
#include "common.h"

#include <algorithm>
#include <functional>
#include <iostream>
#include <optional>

#include <iostream>

using namespace std::literals;


Sheet::~Sheet() {
}

void Sheet::SetCell(Position pos, std::string text) {

    if (!pos.IsValid()) {
        throw InvalidPositionException("SetCell ERROR: InvalidPosition.");
    }

    // Check circular dependencies
    if (!text.empty() && text[0] == '=' && text != "=") {
        auto referenced = ParseFormula(text.substr(1))->GetReferencedCells();
        bool is_cycle = FindCyclicDependencies(referenced, pos);

        if (is_cycle) {
            throw CircularDependencyException("Sheet SetCell ERROR: Formula Circular Dependency");
        }
    }


    auto cell_ptr = dynamic_cast<Cell*>(GetCell(pos));

    // Delete old dependencies if this cell not new
    if (cell_ptr != nullptr) {
        for (const auto& ref_cell_pos : cell_ptr->GetReferencedCells()) {
            auto referenced = dynamic_cast<Cell*>(GetCell(ref_cell_pos));
            if (referenced != nullptr) {
                referenced->DeleteDependency(pos);
            }
        }
    }

    // Create new cell in pos or set old
    if (cell_ptr == nullptr) {

        // resize table for new cell
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

        ptr_table_[pos.row][pos.col] = std::make_unique<Cell>(*this);
        cell_ptr = dynamic_cast<Cell*>(GetCell(pos));
        cell_ptr->Set(text);
    } else {
        cell_ptr->Set(text);
    }

    // Adding New Dependencies after changing formula and refreshing dependent values
    for (const auto& new_cell_ref_pos : cell_ptr->GetReferencedCells()) {
        auto referenced = dynamic_cast<Cell*>(GetCell(new_cell_ref_pos));
        // if referenced to non-existing pos, creates Empty Cell to add dependency
        if (referenced == nullptr) {
            SetCell(new_cell_ref_pos, "");
        }
        // this needed to refresh ptr if empty cell was created
        referenced = dynamic_cast<Cell*>(GetCell(new_cell_ref_pos));

        referenced->AddDependency(pos);
    }

    // Recursive recalculation without itself recalculation
    cell_ptr->RecursiveRecalculateValueCycle();

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
        // Recursive recalculation
        auto cell_ptr = dynamic_cast<Cell*>(GetCell(pos));
        if (cell_ptr != nullptr) {
            cell_ptr->Clear();
        }
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