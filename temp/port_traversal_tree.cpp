#include <iostream>

#include <vector>



class Node
{

public:

    Node()
    {
        currentNode = this;
        //std::cout << currentNode << '\n';
    }

    Node(int in_step, int in_port) : step{in_step}, port{in_port}//TODO добавить для списка инициализации
    {
        currentNode = this;
    }

    void addToSameLevel(int in_step, int in_port)//TODO добавить для списка инициализации
    {
        Node* prevNodeTmp{currentNode->prevNode};
        currentNode->prevNode->nodes.emplace_back(in_step, in_port);
        currentNode->prevNode->nodes.back().prevNode = prevNodeTmp;
        //std::cout << (currentNode + 1)->step << ' ' << (currentNode + 1)->port << ' ' << (currentNode + 1)->prevNode->port << ' ' << (currentNode + 1)->final << '\n';

    }

    void addToAnotherLevel(int in_step, int in_port)//TODO добавить для списка инициализации
    {
        std::cout << currentNode << '\n';
        currentNode->nodes.emplace_back(in_step, in_port);
        std::cout << currentNode << '\n';
        currentNode->nodes[0].prevNode = currentNode;
        //std::cout << currentNode->nodes.front().port;//.back();
        //.prevNode = currentNode;std::cout << "!!!";
        std::cout << "qaz" << '\n';
        currentNode = &currentNode->nodes[0];
        std::cout << "wsx" << '\n';
        std::cout << currentNode << '\n';
        std::cout << currentNode->step << ' ' << currentNode->port << ' ' << currentNode->prevNode->port << ' ' << currentNode->final << '\n';
    }



    Node* getPrevNode()
    {
        return prevNode;
    }

    static void output(Node& in_node)
    {
        /*for (Node& node : in_node.nodes)
        {
            std::cout << node.step << ' ' << node.port << ' ' << node.prevNode->port << ' ' << node.final << '\n';

                output(node);

        }*/

        std::cout << '\n';

        std::cout << in_node.nodes[0].step << ' ' << in_node.nodes[0].port << ' ' << in_node.nodes[0].prevNode->port << ' ' << in_node.nodes[0].final << '\n';
        std::cout << in_node.nodes[1].step << ' ' << in_node.nodes[1].port << ' ' << in_node.nodes[1].prevNode->port << ' ' << in_node.nodes[1].final << '\n';
        std::cout << in_node.nodes[2].step << ' ' << in_node.nodes[2].port << ' ' << in_node.nodes[2].prevNode->port << ' ' << in_node.nodes[2].final << '\n';
        std::cout << in_node.nodes[3].step << ' ' << in_node.nodes[3].port << ' ' << in_node.nodes[3].prevNode->port << ' ' << in_node.nodes[3].final << '\n';
        std::cout << &in_node.nodes[0].nodes[0];
        std::cout << in_node.nodes[0].nodes[0].step << ' ' << in_node.nodes[0].nodes[0].port << ' ' << in_node.nodes[0].nodes[0].prevNode->port << ' ' << in_node.nodes[0].nodes[0].final << '\n';
        //std::cout << in_node.nodes[0].nodes[1].step << ' ' << in_node.nodes[0].nodes[1].port << ' ' << in_node.nodes[0].nodes[1].prevNode->port << ' ' << in_node.nodes[0].nodes[1].final << '\n';
    }

private:

    int step{};
    int port{};
    Node* prevNode{};

    bool final{};

    std::vector<Node> nodes{};

    Node* currentNode;



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

    //std::cout << &node << '\n';

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

    node.addToAnotherLevel(0, 17);

    node.addToSameLevel(0, 16);
    node.addToSameLevel(0, 23);
    node.addToSameLevel(0, 22);

    //Node::getCurrentNode()->add(2, 113);
    //Node::getCurrentNode()->getPrevNode()->add(2, 112);

    node.addToAnotherLevel(2, 113);
    node.addToSameLevel(2, 112);


    //std::cout << '\n';

    //add(node1.nodes[1].nodes, 8, 101);
    //add(node1.nodes[1].nodes, 11, 102);
    //add(node1.nodes[1].nodes[0].nodes, 9, 81);
    //add(node1.nodes[1].nodes[0].nodes, 10, 82);

    Node::output(node);

    std::cout << '\n';


    //std::cout << "Hello, World!" << std::endl;



    return 0;
}
