#pragma once

#include <stdexcept>
#include <tools/graph.hpp>

namespace tools {

template <NodeCtx node_data, EdgeCtx edge_data>
class tree_t : public orgraph_t<node_data, edge_data> {
  public:
    using graph_t = orgraph_t<node_data, edge_data>;
    using node_iter = graph_t::node_iter;
    using edge_iter = graph_t::edge_iter;
    using const_node_iter = graph_t::const_node_iter;
    using const_edge_iter = graph_t::const_edge_iter;

    /**
     * @brief get iterator to the root node
     *
     * WARN: call is invalid, if non of nodes was set as root
     */
    node_iter get_root() noexcept;
    const_node_iter get_root() const noexcept;

    /** @brief set iterator to the root node */
    void set_root(node_iter new_root) noexcept;

    /**
     * @brief [DEBUG] Check, wether tree is not broken
     * @throws std::runtime_error if something went wrong and tree became
     *                            nvalid somehow
     */
    void assert_integrity() const;

  private:
    node_iter root;
};

#define TREE_TEMPLATE template <NodeCtx node_data, EdgeCtx edge_data>
#define TREE_ARGS     node_data, edge_data
#define TREE          tree_t<TREE_ARGS>
#define TREE_EDGE     graph::edge_t<TREE_ARGS>
#define TREE_NODE     graph::node_t<TREE_ARGS>

TREE_TEMPLATE
TREE::node_iter TREE::get_root() noexcept { return root; }

TREE_TEMPLATE
TREE::const_node_iter TREE::get_root() const noexcept { return root; }

TREE_TEMPLATE
void TREE::set_root(node_iter new_root) noexcept { root = new_root; }

TREE_TEMPLATE
void TREE::assert_integrity() const {
    graph_t::assert_integrity();

    if (this->size() == 0) return;

    const_cast<TREE *>(this)->template traverse<graph::DFS, graph::FORWARD>(
        root, [](node_iter n1, edge_iter e, node_iter n2, bool is_visited) {
            if (is_visited) {
                throw std::runtime_error(
                    std::format("Cycle detected: {} visited twice", *n2)
                );
            }
            return true;
        }
    );
}

#undef TREE_TEMPLATE
#undef TREE_ARGS
#undef TREE
#undef TREE_EDGE
#undef TREE_NODE

} // namespace tools
