#include <iostream>

#include <vector>



class Node
{

public:

    Node() = default;

    Node(int in_step, int in_port) : step{in_step}, port{in_port}
    {}



    Node* addToSameLevel(Node& node, int step, int port, Node* prevNode)//TODO добавить для списка инициализации
    {

    }

    void add(int in_step, int in_port)
    {
        nodes.emplace_back(in_step, in_port);
        nodes.back().prevNode = this;

        currentNode = &nodes[0];
        //std::cout << "!!!";
    }

    static Node* getCurrentNode()
    {
        return currentNode;
    }

    Node* getPrevNode()
    {
        return prevNode;
    }

    static void output(Node& in_node)
    {
        for (Node& node : in_node.nodes)
        {
            std::cout << node.step << ' ' << node.port << ' ' << node.prevNode->port << ' ' << node.final << '\n';
            if (!node.nodes.empty())
            {
                output(node);
            }
        }
    }

private:

    int step{};
    int port{};
    Node* prevNode{};

    bool final{};

    std::vector<Node> nodes{};

    inline static Node* currentNode{};



    void setCurrentNode(Node* node)
    {
        currentNode = node;
    }





};



/*std::vector<Node>& searchVector(std::vector<Node>& vec, int outPort)
{
    for (auto& node : vec)
    {
        if (node.port == outPort)
        {
            return node.nodes;
        }
        if (!node.nodes.empty())
        {

            searchVector(node.nodes, outPort);
        }
    }

    return vec;
}*/



int main()
{
    Node node;


/*
    node1.nodes.push_back({12, 92, {}});
    node1.nodes.push_back({7, 91, {}});
    node1.nodes[1].nodes.push_back({8, 101, {}});
    node1.nodes[1].nodes.push_back({11, 102, {}});
    node1.nodes[1].nodes[0].nodes.push_back({9, 81, {}});
    node1.nodes[1].nodes[0].nodes.push_back({10, 82, {}});

    output(node1.nodes);
*/
    //std::cout << '\n';

    node.add(0, 17);
    node.add(0, 16);
    node.add(0, 23);
    node.add(0, 22);

    Node::getCurrentNode()->add(2, 113);
    Node::getCurrentNode()->getPrevNode()->add(2, 112);



    //std::cout << '\n';

    //add(node1.nodes[1].nodes, 8, 101);
    //add(node1.nodes[1].nodes, 11, 102);
    //add(node1.nodes[1].nodes[0].nodes, 9, 81);
    //add(node1.nodes[1].nodes[0].nodes, 10, 82);

    Node::output(node);

    //std::cout << "Hello, World!" << std::endl;



    return 0;
}

