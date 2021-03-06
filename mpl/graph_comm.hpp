#if !(defined MPL_GRAPH_COMM_HPP)

#define MPL_GRAPH_COMM_HPP

#include <mpi.h>
#include <algorithm>
#include <vector>
#include <utility>
#include <set>
#include <tuple>
#include <algorithm>
#include <numeric>

namespace mpl {

  class graph_communicator : public detail::topo_communicator {
  public:
    class edge_set : private std::set<std::pair<int, int>> {
      typedef std::set<std::pair<int, int>> base;
    public:
      using value_type=typename base::value_type;
      using size_type=typename base::size_type;
      using base::base;
      using base::size;
      using base::begin;
      using base::end;
      using base::cbegin;
      using base::cend;
      using base::insert;
      using base::operator=;
    };

    class node_list : private std::vector<int> {
      typedef std::vector<int> base;
    public:
      using value_type=typename base::value_type;
      using size_type=typename base::size_type;
      using base::base;
      using base::size;
      using base::begin;
      using base::end;
      using base::cbegin;
      using base::cend;
      using base::operator=;
      using base::operator[];
      using base::data;
    };

    graph_communicator()=default;

    explicit graph_communicator(const communicator &old_comm,
                                const edge_set &es,
                                bool reorder=true) {
      int nodes=0;
      for (const auto &e : es)
        nodes=std::max({ nodes, e.first+1, e.second+1 });
      int node=0;
      int degree=0;
      std::vector<int> edges, index(nodes, 0);
      for (const auto &e : es) {
        while (e.first>node) {
          ++node;
          degree=0;
        }
        edges.push_back(e.second);
        index[e.first]+=1;
      }
      std::partial_sum(index.begin(), index.end(), index.begin());
      MPI_Graph_create(old_comm.comm, nodes, index.data(), edges.data(), reorder, &comm);
    }

    graph_communicator(graph_communicator &&other) noexcept {
      comm=other.comm;
      other.comm=MPI_COMM_SELF;
    }

    void operator=(const graph_communicator &)= delete;

    graph_communicator &operator=(graph_communicator &&other) noexcept {
      if (this!=&other) {
        int result1, result2;
        MPI_Comm_compare(comm, MPI_COMM_WORLD, &result1);
        MPI_Comm_compare(comm, MPI_COMM_SELF, &result2);
        if (result1!=MPI_IDENT and result2!=MPI_IDENT)
          MPI_Comm_free(&comm);
        comm=other.comm;
        other.comm=MPI_COMM_SELF;
      }
      return *this;
    }

    int neighbors_count(int rank) const {
      int nneighbors;
      MPI_Graph_neighbors_count(comm, rank, &nneighbors);
      return nneighbors;
    };

    int neighbors_count() const {
      return neighbors_count(rank());
    };

    node_list neighbors(int rank) const {
      int maxneighbors=neighbors_count(rank);
      node_list nl(maxneighbors);
      MPI_Graph_neighbors(comm, rank, maxneighbors, nl.data());
      return nl;
    }

    node_list neighbors() const {
      return neighbors(rank());
    }

  };

}

#endif
