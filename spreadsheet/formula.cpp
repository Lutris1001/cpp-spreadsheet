#include "formula.h"

#include "FormulaAST.h"

#include <algorithm>
#include <cassert>
#include <cctype>
#include <sstream>

#include <variant>

using namespace std::literals;

std::ostream& operator<<(std::ostream& output, FormulaError fe) {
    return output << fe.ToString();
}

namespace {
    class Formula : public FormulaInterface {
    public:
    // Реализуйте следующие методы:
    explicit Formula(std::string expression) try
            : ast_(ParseFormulaAST(expression)) {
    } catch (const std::exception& exc) {
        throw(FormulaException(exc.what()));
    }

        Value Evaluate(const SheetInterface& sheet) const override  {

            Value result;

            try {
                result = ast_.Execute([&sheet](Position pos) {

                    if (!pos.IsValid()) {
                        throw FormulaError(FormulaError::Category::Ref);
                    }

                    auto cell = sheet.GetCell(pos);
                    if (cell == nullptr) {
                        return 0.0;
                    }

                    auto cell_value = cell->GetValue();

                    if (std::holds_alternative<double>(cell_value)) {
                        return std::get<double>(cell_value);
                    }

                    if (std::holds_alternative<FormulaError>(cell_value)) {
                        throw std::get<FormulaError>(cell_value);
                    }

                    if (std::holds_alternative<std::string>(cell_value)) {
                        double result = 0.0;
                        std::string str = std::get<std::string>(cell_value);
                        if (str.empty()) {
                            return result;
                        }

                        if (str.find_first_not_of("1234567890.") != str.npos) {
                            throw FormulaError(FormulaError::Category::Value);
                        }

                        try {
                            result = std::stod(str);
                        }
                        catch (...) {
                            throw FormulaError(FormulaError::Category::Value);
                        }
                        return result;
                    }
                    return 0.0;
                }
                );
            }
            catch (FormulaError& err) {
                return err;
            }
            return result;
        }

        std::string GetExpression() const override {
            std::stringstream outline;
            ast_.PrintFormula(outline);
            return outline.str();
        }

        std::vector<Position> GetReferencedCells() const override {
            auto cells = ast_.GetCells();
            std::vector<Position> result(cells.begin(), cells.end());
            auto it = std::unique(result.begin(), result.end());
            result.erase(it, result.end());
            return result;
        }

    private:
        FormulaAST ast_;
    };
}  // namespace

std::unique_ptr<FormulaInterface> ParseFormula(std::string expression) {
    return std::make_unique<Formula>(std::move(expression));
}