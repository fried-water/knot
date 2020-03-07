#include "gtest/gtest.h"

#include "knot.h"

#include <typeindex>
#include <unordered_map>

struct Node {
  int value;
  std::vector<std::shared_ptr<Node>> children;
};

auto as_tie(const Node& node) { return std::tie(node.value, node.children); }

std::shared_ptr<Node> make_node(int val) { return std::make_shared<Node>(Node{val}); }

template <typename T>
std::optional<int> dfs_search(const T& node, int value) {
  std::optional<int> result;

  knot::depth_first_search(node, [&](const Node& node, int id, int /* parent */) {
    if (node.value == value) result = id;
    return node.value == value ? knot::SearchResult::Finish : knot::SearchResult::Explore;
  });

  return result;
}

enum class Topology { Tree, Acyclic, Cyclic};

template <typename T>
Topology topology(const T& t) {
  Topology result = Topology::Tree;

  using ID = std::pair<std::uintptr_t, std::type_index>;
  auto make_id = [](const auto& t) { return ID{reinterpret_cast<std::uintptr_t>(&t), std::type_index{typeid(t)}}; };

  std::unordered_map<int, ID> id_to_ptr;
  std::unordered_map<ID, ID, knot::Hash> child_to_parent;

  knot::depth_first_search(t, [&](const auto& val, int id, int parent) {
    const ID me = make_id(val);

    if (child_to_parent.find(me) == child_to_parent.end()) {
      id_to_ptr.emplace(id, me);
      child_to_parent.emplace(me, parent == -1 ? ID{0, std::type_index(typeid(void))} : id_to_ptr.at(parent));
      return knot::SearchResult::Explore;
    }

    // Check if we have seen ourself in our path
    ID cur = id_to_ptr.at(parent);
    while (cur.first != 0) {
      if (cur == me) {
        result = Topology::Cyclic;
        return knot::SearchResult::Finish;
      }
      cur = child_to_parent.at(cur);
    }

    result = Topology::Acyclic;
    return knot::SearchResult::Reject;
  });

  return result;
}

TEST(DFS, basic) {
  const auto root = make_node(1);
  const auto left = make_node(2);
  const auto right = make_node(3);

  root->children.push_back(left);
  root->children.push_back(right);

  EXPECT_EQ(0, dfs_search(root, 1));
  EXPECT_EQ(1, dfs_search(root, 2));
  EXPECT_EQ(2, dfs_search(root, 3));
  EXPECT_EQ(std::nullopt, dfs_search(root, 4));
}

TEST(DFS, multipath) {
  const auto root = make_node(1);
  const auto n2 = make_node(2);
  const auto n3 = make_node(3);
  const auto dest = make_node(4);

  root->children.push_back(n2);
  root->children.push_back(dest);
  n2->children.push_back(n3);
  n3->children.push_back(dest);

  // Should take long path root -> n2 -> n3 -> dest
  EXPECT_EQ(3, dfs_search(root, 4));
}

TEST(DFS, topology) {
  const auto root = make_node(1);
  const auto n2 = make_node(1);
  const auto n3 = make_node(1);
  const auto n4 = make_node(1);

  root->children.push_back(n2);
  n2->children.push_back(n3);
  n3->children.push_back(n4);

  EXPECT_EQ(Topology::Tree, topology(root));

  // 2 paths to the same node, no longer a tree
  root->children.push_back(n4);
  EXPECT_EQ(Topology::Acyclic, topology(root));

  // Create a loop
  n4->children.push_back(root);
  EXPECT_EQ(Topology::Cyclic, topology(root));
}
