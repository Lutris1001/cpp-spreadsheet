#pragma once

#include "common.h"
#include "formula.h"
#include <sstream>

//using Value = std::variant<std::string, double, FormulaError>;

class Impl {
public:
    Impl() = default;
    virtual ~Impl() = default;
    virtual std::string GetRawValue() = 0;
    virtual CellInterface::Value GetValue() = 0;
    virtual std::vector<Position> GetReferencedCells() const = 0;
    virtual void RecalculateValue() { return; };
};

class EmptyImpl final : public Impl {
public:
    EmptyImpl() = default;

    std::string GetRawValue() override;
    CellInterface::Value GetValue() override;
    std::vector<Position> GetReferencedCells() const override {
        return {};
    }
};

class TextImpl final : public Impl {
public:
    explicit TextImpl(const std::string& text);

    std::string GetRawValue() override;
    CellInterface::Value GetValue() override;
    std::vector<Position> GetReferencedCells() const override {
        return {};
    }
private:
    std::string raw_;
    CellInterface::Value complete_;
};

class FormulaImpl final : public Impl {
public:
    explicit FormulaImpl(const std::string& expression, const SheetInterface& sheet);

    std::string GetRawValue() override;
    CellInterface::Value GetValue() override;
    std::vector<Position> GetReferencedCells() const override;

    // recalculates and assigns new_value;
    void RecalculateValue();

private:
    std::string raw_;
    CellInterface::Value complete_;
    Position this_cell_pos_;
    std::unique_ptr<FormulaInterface> expr_;
    const SheetInterface& sheet_;
};


class Cell : public CellInterface {
public:

    Cell(std::unique_ptr<Impl> content);
    ~Cell();

//    void Set(std::string text);
    void Clear();

    Value GetValue() const override;
    std::string GetText() const override;

    std::vector<Position> GetReferencedCells() const override {
        return impl_->GetReferencedCells();
    }

    // Recalculates this and if new value != old value
    // recursively calls recalculation linked values in other_depends_from_this_;
    void RecalculateValue();

    // Recursive search for already existing cell;
    bool FindCyclicDependencies(Position pos) const;

    // Adds or deletes pos in other_depends_from_this_;
    void AddDependency(Position pos);
    void DeleteDependency(Position pos);

    // Set pos of this in sheet;
    void SetPosition(Position pos);
private:
//можете воспользоваться нашей подсказкой, но это необязательно.
/*    class Impl;
    class EmptyImpl;
    class TextImpl;
    class FormulaImpl;
    std::unique_ptr<Impl> impl_;
*/

    Position pos_; // this Cell pos in sheet;
    std::unique_ptr<Impl> impl_;
    std::vector<Position> this_depends_from_;
    std::vector<Position> other_depends_from_this_;
};

std::unique_ptr<Impl> ParseCellContent(std::string text, const SheetInterface& sheet) {

    if (!text.empty() && text[0] == '=' && text != "=") { // Check expression
        return std::make_unique<FormulaImpl>(text, sheet);
    }

    if (!text.empty()) { // Check expression
        return std::make_unique<TextImpl>(text);
    }

    return std::make_unique<EmptyImpl>();;
}