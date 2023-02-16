#include "cell.h"

#include <cassert>
#include <iostream>
#include <memory>
#include <string>
#include <optional>


std::unique_ptr<Impl> ParseCellContent(std::string text, SheetInterface* sheet) {

    if (!text.empty() && text[0] == '=' && text != "=") { // Check expression
        return std::make_unique<FormulaImpl>(text, sheet);
    }

    if (!text.empty()) { // Check expression
        return std::make_unique<TextImpl>(text);
    }

    return std::make_unique<EmptyImpl>();
}

// Либо текст ячейки, либо значение формулы, либо сообщение об ошибке из
// формулы
using Value = std::variant<std::string, double, FormulaError>;

// Реализуйте следующие методы
Cell::Cell(SheetInterface& sheet)
        : sheet_(&sheet)
{
}

Cell::~Cell() {
}

void Cell::Set(std::string text) {
// Задаёт содержимое ячейки. Если текст начинается со знака "=", то он
    // интерпретируется как формула. Уточнения по записи формулы:
    // * Если текст содержит только символ "=" и больше ничего, то он не считается
    // формулой
    // * Если текст начинается с символа "'" (апостроф), то при выводе значения
    // ячейки методом GetValue() он опускается. Можно использовать, если нужно
    // начать текст со знака "=", но чтобы он не интерпретировался как формула.

    if (!text.empty() && text[0] == '=' && text != "=") { // Check expression
        impl_ = std::make_unique<FormulaImpl>(text, sheet_);
        return;
    }

    if (!text.empty()) { // Check expression
        impl_ = std::make_unique<TextImpl>(text);
        return;
    }

    impl_ = std::make_unique<EmptyImpl>();

}

void Cell::AddDependency(Position pos) {
    auto it = std::find(depends_from_this_.begin(), depends_from_this_.end(), pos);
    if (it == depends_from_this_.end()) {
        depends_from_this_.push_back(pos);
    }
}

void Cell::DeleteDependency(Position pos) {
    auto it = std::find(depends_from_this_.begin(), depends_from_this_.end(), pos);
    if (it != depends_from_this_.end()) {
        depends_from_this_.erase(it);
    }
}

void Cell::RecursiveRecalculateValueCycle() {
    for (const auto& cell : depends_from_this_) {
        auto ptr = dynamic_cast<Cell*>(sheet_->GetCell(cell));
        ptr->RecursiveRecalculateValue();
    }
}

void Cell::RecursiveRecalculateValue() {

    impl_->CalculateValue();
    RecursiveRecalculateValueCycle();
}


void Cell::Clear() {
    impl_ = std::make_unique<EmptyImpl>();
    RecursiveRecalculateValueCycle();
}

Cell::Value Cell::GetValue() const {
    // Возвращает видимое значение ячейки.
    // В случае текстовой ячейки это её текст (без экранирующих символов). В
    // случае формулы - числовое значение формулы или сообщение об ошибке.
    return impl_->GetValue();
}

std::string Cell::GetText() const {
    // Возвращает внутренний текст ячейки, как если бы мы начали её
    // редактирование. В случае текстовой ячейки это её текст (возможно,
    // содержащий экранирующие символы). В случае формулы - её выражение.
    return impl_->GetRawValue();
}



std::string EmptyImpl::GetRawValue() {
    return "";
}
Value EmptyImpl::GetValue() {
    return "";
}


TextImpl::TextImpl(const std::string& text)
        : raw_(text)
{
    complete_ = !text.empty() && text[0] == '\'' ? text.substr(1) : text;
}

std::string TextImpl::GetRawValue() {
    return raw_;
}
Value TextImpl::GetValue(){
    return complete_;
}


FormulaImpl::FormulaImpl(const std::string& expression, SheetInterface* sheet)
    : sheet_(sheet)
{
    expr_ = ParseFormula(expression.substr(1));

    raw_ = "=" + expr_->GetExpression();

    CalculateValue();
}

void FormulaImpl::CalculateValue() {

    auto result = expr_->Evaluate(*sheet_);

    if (std::holds_alternative<FormulaError>(result)) {
        complete_ = std::get<FormulaError>(result);
        return;
    }

    if (std::holds_alternative<double>(result)) {
        complete_ = std::get<double>(result);
        return;
    }
}

std::string FormulaImpl::GetRawValue() {
    return raw_;
}
Value FormulaImpl::GetValue() {
    return complete_;
}

std::vector<Position> FormulaImpl::GetReferencedCells() const {
    return expr_->GetReferencedCells();
}

std::vector<Position> Cell::GetReferencedCells() const {
    return impl_->GetReferencedCells();
}