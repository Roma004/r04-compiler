#include "frontend/ast/ast.hpp"

namespace frontend::ast {

std::string Ast::repr_node(const const_node_iter &node) const {
    const auto &data = node->get_data();
    return std::visit(
        [&](const auto &v) -> std::string {
            std::string s = v.repr();
            if constexpr (types::is_expr_v<std::remove_cvref_t<decltype(v)>>) {
                if (data.type)
                    return std::format("{}|{}", data.type->repr(types), s);
                return std::format("<?>|{}", s);
            } else {
                return s;
            }
        },
        data.val
    );
}

std::string Ast::repr_edge(const const_edge_iter &edge) const {
    const node_t &node = edge->source()->get_data();
    const int d = edge->get_data().data;
    return std::visit(
        [d](const auto &v) -> std::string { return v.edge_name(d); }, node.val
    );
}

} // namespace frontend::ast