#pragma once

#include "cell.h"
#include "common.h"

#include <functional>

class Sheet : public SheetInterface {
public:
    ~Sheet();

    void SetCell(Position pos, std::string text) override;

    const CellInterface* GetCell(Position pos) const override;
    CellInterface* GetCell(Position pos) override;

    void ClearCell(Position pos) override;

    Size GetPrintableSize() const override;

    void PrintValues(std::ostream& output) const override;
    void PrintTexts(std::ostream& output) const override;

    // Recursive search for pos in Formula GetReferencedCells() for non-existing cell before creating cell
    // cause Formula can be incorrect;
    bool FindCyclicDependencies(const std::vector<Position>& previous_cells, Position pos) const;

private:
    // Можете дополнить ваш класс нужными полями и методами
    using Table = std::vector<std::vector<std::unique_ptr<CellInterface>>>;
    Table ptr_table_;

    int max_width_ = 0;
    int max_height_ = 0;

    void FindAndDecreaseMaxHeightAndWidth();
};