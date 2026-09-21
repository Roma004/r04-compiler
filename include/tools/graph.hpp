#pragma once
#include <concepts>
#include <cstdint>
#include <deque>
#include <functional>
#include <map>
#include <ostream>
#include <set>
#include <stddef.h>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

#include "blocklist.hpp"
#include "macro_template.hpp"

namespace tools {

template <typename T>
concept NodeCtx = requires {
    std::is_copy_constructible_v<T> && std::is_copy_assignable_v<T>;
};

template <typename T>
concept EdgeCtx = requires {
    std::is_copy_constructible_v<T> && std::is_copy_assignable_v<T>;
};

#define GRAPH_TEMPLATE template <NodeCtx node_data, EdgeCtx edge_data>
#define GRAPH_ARGS     node_data, edge_data
#define GRAPH          orgraph_t<GRAPH_ARGS>
#define GRAPH_EDGE     edge_t<GRAPH_ARGS>
#define GRAPH_NODE     node_t<GRAPH_ARGS>

GRAPH_TEMPLATE class orgraph_t;

namespace graph {

constexpr static const size_t nodes_block_size = 64;

enum InspectType { DFS, BFS };
enum InspectDirection { FORWARD, BACKWARD };

GRAPH_TEMPLATE struct node_t;
GRAPH_TEMPLATE struct edge_t;

GRAPH_TEMPLATE
using node_container_t = blocklist<graph::node_t<GRAPH_ARGS>, nodes_block_size>;

GRAPH_TEMPLATE
using edge_container_t = blocklist<graph::edge_t<GRAPH_ARGS>, nodes_block_size>;

GRAPH_TEMPLATE
using node_iterator = typename node_container_t<GRAPH_ARGS>::iterator;

GRAPH_TEMPLATE
using edge_iterator = typename edge_container_t<GRAPH_ARGS>::iterator;

GRAPH_TEMPLATE
using const_node_iterator =
    typename node_container_t<GRAPH_ARGS>::const_iterator;

GRAPH_TEMPLATE
using const_edge_iterator =
    typename edge_container_t<GRAPH_ARGS>::const_iterator;

} // namespace graph

GRAPH_TEMPLATE
class orgraph_t {
  public:
    using node_iter = graph::node_iterator<GRAPH_ARGS>;
    using edge_iter = graph::edge_iterator<GRAPH_ARGS>;
    using const_node_iter = graph::const_node_iterator<GRAPH_ARGS>;
    using const_edge_iter = graph::const_edge_iterator<GRAPH_ARGS>;

    virtual ~orgraph_t() = default;

    node_iter begin() noexcept;
    node_iter end() noexcept;
    const_node_iter begin() const noexcept;
    const_node_iter end() const noexcept;

    /**
     * @brief Insert new node witout any links
     *
     * @param  args  Arguments forwarded to node_data's constructor
     * @return iterator to the node created
     */
    template <typename... Args>
        requires std::constructible_from<node_data, Args...>
    node_iter emplace_node(Args &&...args);

    /**
     * @brief Insert an edge from @p source into @p target
     *
     * @param  source  Iterator to the source of new edge
     * @param  target  Iterator to the target of new edge
     * @param  args    Arguments forwarded to edge_data's constructor
     * @return iterator to the edge created
     * @throws std::logic_error if edge source->target already exists in graph
     */
    template <typename... Args>
        requires std::constructible_from<edge_data, Args...>
    edge_iter emplace_edge(node_iter source, node_iter target, Args &&...args);

    /**
     * @brief Change the target of the edge
     *
     * @param  edge        Iterator to the edge need to be changes
     * @param  new_target  Iterator to node need to be set as target of @p edge
     * @throws std::logic_error if edge source->new_target already exists
     */
    void set_egde_target(edge_iter edge, node_iter new_target);

    /**
     * @brief Change the source of the edge
     *
     * @param  edge        Iterator to the edge need to be changes
     * @param  new_source  Iterator to node need to be set as source of @p edge
     * @throws std::logic_error if edge new_source->target already exists
     */
    void set_egde_source(edge_iter edge, node_iter new_source);

    /**
     * @brief Remove the node by it's iterator
     *
     * This operation leads to the cascade removal of all edges that starts or
     * ends with this node
     */
    void remove_node(node_iter);

    /** @brief Remove the edge by it's iterator */
    void remove_edge(edge_iter);

    size_t size() const noexcept;

    /**
     * @brief [DEBUG] Check, that graph is not broken
     * @throws std::runtime_error if something went wrong and graph became
     *                            nvalid somehow
     */
    void assert_integrity() const;

    /**
     * @brief Inspect the graph
     *
     * A way to iterate over the graph nodes in BFS or DFS order by either
     * forward or backward node connections.
     *
     * @tparam type  type of traverse: (`graph::DFS`, `graph::BFS`)
     * @tparam dir   direction of the traverse:
     *               (`graph::FORWARD`, `graph::BACKWARD`)
     *
     * @param start   iterator to the node, whic his start of traverse
     * @param handle  element handling function
     *
     * Handler arguments are:
     * - (node1)       node which edges are handeled now (always black)
     * - (edge)        data of the edge between node1 and node2
     * - (node2)       newely discovered node on the other side of the edge
     * - (is_visited)  the color of the node2 (
     *                   true  => in queue and might be handeled already
     *                   false => just added in queue,
     *                 )
     *
     * It means:
     * - if dir == FORWARD,  node1 == edge->source(), node2 = edge->target()
     * - if dir == BACKWARD, node1 == edge->taRGET(), node2 = edge->source()
     *
     * Handler returns status: wether to continue traverse (false => stop)
     */
    template <
        graph::InspectType type = graph::DFS,
        graph::InspectDirection dir = graph::FORWARD>
    void traverse(
        node_iter start,
        std::function<bool(node_iter, edge_iter, node_iter, bool)> handle
    );

    /** @brief Overload of traverse, but iterate data, not iterators */
    template <
        graph::InspectType type = graph::DFS,
        graph::InspectDirection dir = graph::FORWARD>
    void traverse(
        node_iter start,
        std::function<bool(node_data &, edge_data &, node_data &, bool)> handle
    );

    /** @brief Constant overload of traverse, but iterate data, not iterators */
    template <
        graph::InspectType type = graph::DFS,
        graph::InspectDirection dir = graph::FORWARD>
    void traverse(
        node_iter start,
        std::function<
            bool(const node_data &, const edge_data &, const node_data &, bool)>
            handle
    ) const;

    /**
     * @brief Print graph into the stream with DOT format
     *
     * Gets node_data and edge_data converting functions, which should return
     * a vector of node or edge attributes.
     * */
    template <typename NodeConv, typename EdgeConv>
        requires helpers::DOTConverter<node_data, NodeConv>
              && helpers::DOTConverter<edge_data, EdgeConv>
    void to_dot(std::ostream &out, NodeConv nc, EdgeConv ec) const noexcept;

  private:
    graph::node_container_t<GRAPH_ARGS> nodes_list;
    graph::edge_container_t<GRAPH_ARGS> edges_list;
};

namespace graph {

GRAPH_TEMPLATE struct edge_t {
    using node_iter = node_iterator<GRAPH_ARGS>;

    template <typename... Args>
        requires std::constructible_from<edge_data, Args...>
    edge_t(node_iter source, node_iter target, Args &&...args);

    edge_data &get_data() noexcept;
    const edge_data &get_data() const noexcept;

    node_iter source() const noexcept;
    node_iter target() const noexcept;

    std::string repr() const noexcept;

  private:
    edge_data data;
    node_iter source_node;
    node_iter target_node;

    friend class orgraph_t<GRAPH_ARGS>;
};

GRAPH_TEMPLATE struct node_t {
    using edge_iter = edge_iterator<GRAPH_ARGS>;
    using node_iter = node_iterator<GRAPH_ARGS>;

    using edges_list_t = std::vector<edge_iter>;
    using edge_position = typename edges_list_t::const_iterator;

    template <typename... Args>
        requires std::constructible_from<node_data, Args...>
    node_t(Args &&...args);

    node_data &get_data() noexcept;
    const node_data &get_data() const noexcept;

    const edges_list_t &forward_edges() const noexcept;
    const edges_list_t &backward_edges() const noexcept;

    edge_position get_edge_target(node_iter target) const noexcept;
    edge_position get_edge_source(node_iter target) const noexcept;

    std::string repr() const noexcept;

  private:
    node_data data;
    edges_list_t forward_list;
    edges_list_t backward_list;

    friend class orgraph_t<GRAPH_ARGS>;
};

} // namespace graph

// ---------------------------------------------------------------------------
// edge_t implementation
// ---------------------------------------------------------------------------

namespace graph {

GRAPH_TEMPLATE
template <typename... Args>
    requires std::constructible_from<edge_data, Args...>
GRAPH_EDGE::edge_t(node_iter source, node_iter target, Args &&...args) :
    data(std::forward<Args...>(args...)), source_node(source),
    target_node(target) {}

GRAPH_TEMPLATE
edge_data &GRAPH_EDGE::edge_t::get_data() noexcept { return data; }

GRAPH_TEMPLATE
const edge_data &GRAPH_EDGE::get_data() const noexcept { return data; }

GRAPH_TEMPLATE
node_iterator<GRAPH_ARGS> GRAPH_EDGE::target() const noexcept {
    return target_node;
}

GRAPH_TEMPLATE
node_iterator<GRAPH_ARGS> GRAPH_EDGE::source() const noexcept {
    return source_node;
}

GRAPH_TEMPLATE
std::string GRAPH_EDGE::repr() const noexcept {
    if constexpr (helpers::Stringifiable<edge_data>) {
        using std::to_string;
        return to_string(data);
    } else if constexpr (std::is_convertible_v<edge_data, std::string_view>) {
        return std::string(data);
    } else {
        return "edge[0x" + std::to_string((uintptr_t)&data) + "]";
    }
}

}; // namespace graph

// ---------------------------------------------------------------------------
// node_t implementation
// ---------------------------------------------------------------------------

namespace graph {

GRAPH_TEMPLATE
template <typename... Args>
    requires std::constructible_from<node_data, Args...>
GRAPH_NODE::node_t(Args &&...args) :
    data(std::forward<Args &&...>(args...)), backward_list(), forward_list() {}

GRAPH_TEMPLATE
node_data &GRAPH_NODE::get_data() noexcept { return data; }

GRAPH_TEMPLATE
const node_data &GRAPH_NODE::get_data() const noexcept { return data; }

GRAPH_TEMPLATE
const typename GRAPH_NODE::edges_list_t &
GRAPH_NODE::forward_edges() const noexcept {
    return forward_list;
}

GRAPH_TEMPLATE
const typename GRAPH_NODE::edges_list_t &
GRAPH_NODE::backward_edges() const noexcept {
    return backward_list;
}

GRAPH_TEMPLATE
typename GRAPH_NODE::edge_position
GRAPH_NODE::get_edge_target(node_iter target) const noexcept {
    for (auto it = forward_list.begin(); it != forward_list.end(); ++it)
        if ((*it)->target() == target) return it;
    return forward_list.end();
}

GRAPH_TEMPLATE
typename GRAPH_NODE::edge_position
GRAPH_NODE::get_edge_source(node_iter source) const noexcept {
    for (auto it = backward_list.begin(); it != backward_list.end(); ++it)
        if ((*it)->source() == source) return it;
    return backward_list.end();
}

GRAPH_TEMPLATE
std::string GRAPH_NODE::repr() const noexcept {
    if constexpr (helpers::Stringifiable<node_data>) {
        using std::to_string;
        return to_string(data);
    } else if constexpr (std::is_convertible_v<node_data, std::string_view>) {
        return std::string(data);
    } else {
        return "node[0x" + std::to_string((uintptr_t)&data) + "]";
    }
}

} // namespace graph

// ---------------------------------------------------------------------------
// orgraph_t implementation
// ---------------------------------------------------------------------------

GRAPH_TEMPLATE
typename GRAPH::node_iter GRAPH::begin() noexcept { return nodes_list.begin(); }

GRAPH_TEMPLATE
typename GRAPH::node_iter GRAPH::end() noexcept { return nodes_list.end(); }

GRAPH_TEMPLATE
typename GRAPH::const_node_iter GRAPH::begin() const noexcept {
    return nodes_list.begin();
}

GRAPH_TEMPLATE
typename GRAPH::const_node_iter GRAPH::end() const noexcept {
    return nodes_list.end();
}

GRAPH_TEMPLATE
template <typename... Args>
    requires std::constructible_from<node_data, Args...>
typename GRAPH::node_iter GRAPH::emplace_node(Args &&...args) {
    return nodes_list.emplace(std::forward<Args &&>(args)...);
}

GRAPH_TEMPLATE
template <typename... Args>
    requires std::constructible_from<edge_data, Args...>
typename GRAPH::edge_iter
GRAPH::emplace_edge(node_iter source, node_iter target, Args &&...args) {
    // if edge between `from` and `to` already exists do not create a new one
    auto edge_o = source->get_edge_target(target);
    if (edge_o != source->forward_edges().end()) {
        throw std::logic_error(
            std::format(
                "Can't insert the egde. duplicates existing edge `{}`[{} --> "
                "{}]",
                *(*edge_o),
                *(*edge_o)->source(),
                *(*edge_o)->target()
            )
        );
    }

    // if not, create new edge and add it into edges lists of from and to
    edge_iter edge =
        edges_list.emplace(source, target, std::forward<Args &&>(args)...);

    source->forward_list.emplace_back(edge);
    target->backward_list.emplace_back(edge);
    return edge;
}

GRAPH_TEMPLATE
void GRAPH::set_egde_target(edge_iter edge, node_iter new_target) {
    auto old_target = edge->target();
    auto source = edge->source();

    auto pos = new_target->get_edge_source(source);
    if (pos == new_target->backward_list.end()) {
        // if there is no edge source->new_target, update this edge:
        // remove it from old_target's backward_list
        old_target->backward_list.erase(old_target->get_edge_source(source));

        // add it to new_target's backward_list
        new_target->backward_list.emplace_back(edge);

        // change the target
        edge->target_node = new_target;
    } else {
        auto &dup_e = *pos;
        throw std::logic_error(
            std::format(
                "can't change target of edge `{}`[{} --> {}]. Such operation"
                " will duplicate existing edge `{}`[{} --> {}]",
                *edge,
                *edge->source(),
                *edge->target(),
                *dup_e,
                *dup_e->source(),
                *dup_e->target()
            )
        );
    }
}

GRAPH_TEMPLATE
void GRAPH::set_egde_source(edge_iter edge, node_iter new_source) {
    auto old_source = edge->source();
    auto target = edge->target();

    auto pos = new_source->get_edge_target(target);
    if (pos == new_source->forward_list.end()) {
        // if there is no edge target->new_source, update this edge:
        // remove it to old_source's forward_list
        old_source->forward_list.erase(old_source->get_edge_target(target));

        // add it from new_source's forward_list
        new_source->forward_list.emplace_back(edge);

        // change the source
        edge->source_node = new_source;
    } else {
        auto &dup_e = *pos;
        throw std::logic_error(
            std::format(
                "can't change source of edge `{}`[{} --> {}]. Such operation"
                " will duplicate existing edge `{}`[{} --> {}]",
                *edge,
                *edge->source(),
                *edge->target(),
                *dup_e,
                *dup_e->source(),
                *dup_e->target()
            )
        );
    }
}

GRAPH_TEMPLATE
void GRAPH::remove_node(node_iter node) {
    for (auto e : node->forward_edges()) remove_edge(e);
    for (auto e : node->backward_edges()) remove_edge(e);
    nodes_list.erase(node);
}

GRAPH_TEMPLATE
void GRAPH::remove_edge(edge_iter edge) {
    edge->source()->forward_list.erase(
        edge->source()->get_edge_target(edge->target())
    );
    edge->target()->backward_list.erase(
        edge->target()->get_edge_source(edge->source())
    );
    edges_list.erase(edge);
}

GRAPH_TEMPLATE
template <graph::InspectType type, graph::InspectDirection dir>
void GRAPH::traverse(
    node_iter start,
    std::function<bool(node_iter, edge_iter, node_iter, bool)> handle
) {
    std::set<graph::GRAPH_NODE *> visited;
    std::deque<node_iter> seq;

    auto insert = [&visited, &seq](node_iter it) {
        auto *ptr = &*it;
        if (visited.find(ptr) != visited.end()) return false;
        visited.insert(ptr);
        if constexpr (type == graph::BFS) {
            seq.push_back(it);
        } else {
            seq.push_front(it);
        }
        return true;
    };

    insert(start);

    while (!seq.empty()) {
        auto node = seq.front();
        seq.pop_front();

        const typename graph::node_t<GRAPH_ARGS>::edges_list_t *edges = nullptr;
        if constexpr (dir == graph::FORWARD) {
            edges = &node->forward_edges();
        } else {
            edges = &node->backward_edges();
        }
        for (auto e : *edges) {
            bool stop_traverse = false;
            if constexpr (dir == graph::FORWARD) {
                bool is_visited = !insert(e->target());
                stop_traverse =
                    !handle(e->source(), e, e->target(), is_visited);
            } else {
                bool is_visited = !insert(e->source());
                stop_traverse =
                    !handle(e->target(), e, e->source(), is_visited);
            }
            if (stop_traverse) return;
        }
    }
}

GRAPH_TEMPLATE
template <graph::InspectType type, graph::InspectDirection dir>
void GRAPH::traverse(
    node_iter start,
    std::function<bool(node_data &, edge_data &, node_data &, bool)> handle
) {
    traverse<type, dir>(
        start,
        [&handle](
            node_iter node1, edge_iter edge, node_iter node2, bool is_visited
        ) {
            return handle(
                node1->get_data(),
                edge->get_data(),
                node2->get_data(),
                is_visited
            );
        }
    );
}

GRAPH_TEMPLATE
template <graph::InspectType type, graph::InspectDirection dir>
void GRAPH::traverse(
    node_iter start,
    std::function<
        bool(const node_data &, const edge_data &, const node_data &, bool)>
        handle
) const {
    const_cast<GRAPH *>(this)->traverse<type, dir>(
        start,
        [&handle](
            node_iter node1, edge_iter edge, node_iter node2, bool is_visited
        ) {
            return handle(
                node1->get_data(),
                edge->get_data(),
                node2->get_data(),
                is_visited
            );
        }
    );
}

GRAPH_TEMPLATE
template <typename NodeConv, typename EdgeConv>
    requires helpers::DOTConverter<node_data, NodeConv>
          && helpers::DOTConverter<edge_data, EdgeConv>
void GRAPH::to_dot(std::ostream &out, NodeConv nc, EdgeConv ec) const noexcept {
    std::map<const node_data *, std::string> node_names;

    out << "digraph {\n";

    size_t node_id = 0;
    for (auto &n : nodes_list) {
        auto node_name = "N" + std::to_string(node_id++);
        node_names[&n.get_data()] = node_name;
        out << "  " << node_name << " [";
        for (auto &&[key, value] : nc(n.get_data())) {
            out << std::format("{}=\"{}\",", key, value);
        }
        out << "]\n";
    }
    for (auto &e : edges_list) {
        out << "  " << node_names[&e.source()->get_data()] << " -> "
            << node_names[&e.target()->get_data()] << " [";
        for (auto &&[key, value] : ec(e.get_data())) {
            out << std::format("{}=\"{}\",", key, value);
        }
        out << "]\n";
    }

    out << "}\n";
}

GRAPH_TEMPLATE void GRAPH::assert_integrity() const {
    struct check_row {
        const graph::GRAPH_EDGE *e;
        const graph::GRAPH_NODE *target;
        const graph::GRAPH_NODE *source;
    };
    std::map<uintptr_t, check_row> check_map;

    // fill map with edges data of all registered edges
    for (auto &e : edges_list)
        check_map[(uintptr_t)&e] = check_row{&e, nullptr, nullptr};

    // iterate over every nodes forwards and backwards to find any issues:
    // - edge must be present in exactly one of backward and forward lists
    // - there must not be any edge in lists, that is not in check_map
    // - ends of edge must be the same nodes, that was found during lists loopup
    for (auto &n : nodes_list) {
        for (auto &e : n.forward_edges()) {
            uintptr_t key = (uintptr_t)&*e;
            if (check_map.find(key) == check_map.end())
                throw std::runtime_error(
                    std::format("{} has unregistered forward edge", n)
                );
            if (check_map[key].source != nullptr)
                throw std::runtime_error(
                    std::format(
                        "{} is forward for both {} and {}",
                        *e,
                        n,
                        *check_map[key].source
                    )
                );
            check_map[key].source = &n;
        }
        for (auto &e : n.backward_edges()) {
            uintptr_t key = (uintptr_t)&*e;
            if (check_map.find(key) == check_map.end())
                throw std::runtime_error(
                    std::format("{} has unregistered backward edge", n)
                );
            if (check_map[key].target != nullptr)
                throw std::runtime_error(
                    std::format(
                        "{} is backward for both {} and {}",
                        *e,
                        n,
                        *check_map[key].target
                    )
                );
            check_map[key].target = &n;
        }
    }

    for (auto &&[key, row] : check_map) {
        if (row.target == nullptr)
            throw std::runtime_error(
                std::format(
                    "{} is not present in any of backward lists", *row.e
                )
            );
        if (row.source == nullptr)
            throw std::runtime_error(
                std::format("{} is not present in any of forward lists", *row.e)
            );
        if (&*row.e->target() != row.target)
            throw std::runtime_error(
                std::format(
                    "target of {} is {}, however it is backward of {}",
                    *row.e,
                    *row.e->target(),
                    *row.target
                )
            );
        if (&*row.e->source() != row.source)
            throw std::runtime_error(
                std::format(
                    "source of {} is {}, however it is forward of {}",
                    *row.e,
                    *row.e->source(),
                    *row.source
                )
            );
    }
}

GRAPH_TEMPLATE
size_t GRAPH::size() const noexcept { return nodes_list.size(); }

#undef GRAPH_TEMPLATE
#undef GRAPH
#undef GRAPH_ARGS
#undef GRAPH_EDGE
#undef GRAPH_NODE

} // namespace tools
