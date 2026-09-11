#include <iostream>

#include <list>



struct Link
{
    int port1;
    int port2;
};

struct NodesAndPorts
{

    //NodesAndPorts() = default;

    //NodesAndPorts(int in_node) : node{in_node}
    //{}

    /*void addNodeAndPort(int in_node, int in_port)
    {
        nodesAndPorts.emplace_back<NodeAndPort>({in_node, in_port});
    }*/

    struct NodeAndPort
    {
        int node;
        int port;
    };

    int node{};

    std::list<NodeAndPort> nodesAndPorts;
};

struct Node
{
    //Node() = default;

    //Node(int in_step, int in_port) : step{in_step}, port{in_port}//TODO добавить для списка инициализации
    //{}

    //friend BypassingPorts;

    int step{};
    int port{};

    Node* prevNode{};

    bool final{};

    std::list<Node> nodes{};

    std::list<NodesAndPorts> nodesAndPorts;
};



class BypassingPorts
{

public:

    BypassingPorts()
    {
        currentNode = &node;
    }

    BypassingPorts(int in_initialNode, std::list<NodesAndPorts>&& in_nodesAndPorts)
    {
        currentNode = &node;
        addNodeAndPort(in_initialNode, std::move(in_nodesAndPorts));
    }

    void addToCurrentLevel(int in_step, int in_port)//TODO добавить для списка инициализации
    {
        Node* prevNodeTmp{currentNode->prevNode};
        currentNode->prevNode->nodes.emplace_back<Node>({in_step, in_port});
        currentNode->prevNode->nodes.back().prevNode = prevNodeTmp;
    }

    void addToNewLevel(int in_step, int in_port)//TODO добавить для списка инициализации
    {
        currentNode->nodes.emplace_back<Node>({in_step, in_port});
        currentNode->nodes.front().prevNode = currentNode;
        currentNode = &currentNode->nodes.front();
    }

    void addNodeAndPort(int in_initialNode, std::list<NodesAndPorts>&& in_nodesAndPorts)//TODO сделать внешнюю функцию для создания списка
    {
        initialNode = in_initialNode;
        currentNode->nodesAndPorts = std::move(in_nodesAndPorts);
    }

    std::list<Link> run()
    {

    }

    int nodeDone()
    {
        currentNode->final = true;

        if (currentNode == &node)
        {
            return currentNode->port;
        }

        for (auto it{currentNode->prevNode->nodes.begin()}; it != currentNode->prevNode->nodes.end(); ++it)
        {
            if (currentNode == &*it)
            {
                if (++it != currentNode->prevNode->nodes.end())
                {
                    currentNode = &*(it);
                    if (!currentNode->nodes.empty())
                    {
                        currentNode = &currentNode->nodes.front();
                    }

                } else
                {
                    currentNode = currentNode->prevNode;
                    nodeDone();
                }

                break;
            }
        }

        return currentNode->port;
    }

    int getCurrentPort()
    {
        return currentNode->port;
    }

    int getPrevStep()
    {
        return currentNode->prevNode->step;
    }



    void output(Node& in_node)
    {
        for (Node &node: in_node.nodes)
        {
            std::cout << node.step << ' ' << node.port << ' ' << node.prevNode->port << ' ' << node.final << '\n';

            output(node);
        }
    }

    void outputAll(BypassingPorts& bypassingPorts)
    {
        std::cout  << '\n' << "Port traversal Tree" << '\n';

        std::cout << bypassingPorts.node.step << ' ' << bypassingPorts.node.port << ' ' << '0' << ' ' << bypassingPorts.node.final << '\n';

        output(bypassingPorts.node);
    }

private:

    Node node;

    Node* currentNode;

    int initialNode;

    std::list<Link> links;

};



int main()
{
    BypassingPorts bypassingPorts;

    bypassingPorts.addToNewLevel(0, 17);
    bypassingPorts.addToCurrentLevel(0, 16);
    bypassingPorts.addToCurrentLevel(0, 23);
    bypassingPorts.addToCurrentLevel(0, 22);

    bypassingPorts.addToNewLevel(2, 113);
    bypassingPorts.addToCurrentLevel(2, 112);

    bypassingPorts.addToNewLevel(3, 131);
    bypassingPorts.addToCurrentLevel(3, 132);

    bypassingPorts.nodeDone();
    bypassingPorts.nodeDone();

    bypassingPorts.addToNewLevel(6, 122);
    bypassingPorts.addToCurrentLevel(6, 121);
    bypassingPorts.addToCurrentLevel(6, 1224);

    bypassingPorts.nodeDone();
    bypassingPorts.nodeDone();
    //bypassingPorts.nodeDone();
    //bypassingPorts.nodeDone();
    //bypassingPorts.nodeDone();
    //bypassingPorts.nodeDone();

    bypassingPorts.outputAll(bypassingPorts);

    return 0;
}
