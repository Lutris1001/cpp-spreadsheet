#pragma once

#include "common.h"
#include "formula.h"

#include <functional>
#include <unordered_set>

class Sheet;

class Impl {
public:
    Impl() = default;
    virtual ~Impl() = default;
    virtual std::string GetRawValue() = 0;
    virtual CellInterface::Value GetValue() = 0;
    virtual std::vector<Position> GetReferencedCells() const = 0;
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
    explicit FormulaImpl(const std::string& expression, SheetInterface* sheet);

    std::string GetRawValue() override;
    CellInterface::Value GetValue() override;
    std::vector<Position> GetReferencedCells() const override;

private:
    SheetInterface* sheet_;
    std::string raw_;
    CellInterface::Value complete_;
    Position this_cell_pos_;
    std::unique_ptr<FormulaInterface> expr_;
};


class Cell : public CellInterface {
public:
    Cell(SheetInterface& sheet);
    ~Cell();

    void Set(std::string text);
    void Clear();

    Value GetValue() const override;
    std::string GetText() const override;
    std::vector<Position> GetReferencedCells() const override;

    bool IsReferenced() const;

private:
    SheetInterface* sheet_;
    std::unique_ptr<Impl> impl_;
};
