#include <iostream>

#include <vector>
#include <list>



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

    void addToCurrentLevel(int in_step, int in_port)//TODO добавить для списка инициализации
    {
        Node* prevNodeTmp{currentNode->prevNode};
        currentNode->prevNode->nodes.emplace_back(in_step, in_port);
        currentNode->prevNode->nodes.back().prevNode = prevNodeTmp;
        //std::cout << (currentNode + 1)->step << ' ' << (currentNode + 1)->port << ' ' << (currentNode + 1)->prevNode->port << ' ' << (currentNode + 1)->final << '\n';

    }

    void addToNewLevel(int in_step, int in_port)//TODO добавить для списка инициализации
    {
        std::cout << currentNode << '\n';
        currentNode->nodes.emplace_back(in_step, in_port);
        std::cout << currentNode << '\n';
        currentNode->nodes.front().prevNode = currentNode;
        //std::cout << currentNode->nodes.front().port;//.back();
        //.prevNode = currentNode;std::cout << "!!!";
        std::cout << "qaz" << '\n';
        currentNode = &currentNode->nodes.front();
        std::cout << "wsx" << '\n';
        std::cout << currentNode << '\n';
        std::cout << currentNode->step << ' ' << currentNode->port << ' ' << currentNode->prevNode->port << ' ' << currentNode->final << '\n';


    }



    int nodeDone()
    {
        currentNode->final = true;

        bool lastNode{};
        for (auto it{currentNode->prevNode->nodes.begin()}; it != currentNode->prevNode->nodes.end(); ++it)
        {
            if (currentNode == &*it)
            {
                if (++it != currentNode->prevNode->nodes.end())
                {
                    currentNode = &*(it);

                } else
                {
                    lastNode = true;
                }

                break;
            }
        }

        if (lastNode)
        {
            currentNode = currentNode->prevNode;
            nodeDone();
        }

        return currentNode->port;
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

            output(node);

        }

        //std::cout << "size of in_node.nodes: " << in_node.nodes.size() << '\n';
        //std::cout << "capacity of in_node.nodes: " << in_node.nodes.capacity() << '\n';
        /*if (!in_node.nodes[11].nodes.empty())
        {

            std::cout << "Not empty!" << '\n';
        }*/

        /*std::cout << "size of in_node.nodes[0].nodes: " << in_node.nodes.  [0].nodes.size() << '\n';
        std::cout << "size of in_node.nodes[1].nodes: " << in_node.nodes[1].nodes.size() << '\n';
        std::cout << "size of in_node.nodes[2].nodes: " << in_node.nodes[2].nodes.size() << '\n';
        std::cout << "size of in_node.nodes[3].nodes: " << in_node.nodes[3].nodes.size() << '\n';
        std::cout << "size of in_node.nodes[4].nodes: " << in_node.nodes[4].nodes.size() << '\n';
        std::cout << "size of in_node.nodes[5].nodes: " << in_node.nodes[5].nodes.size() << '\n';
        std::cout << "size of in_node.nodes[6].nodes: " << in_node.nodes[6].nodes.size() << '\n';
        std::cout << "size of in_node.nodes[7].nodes: " << in_node.nodes[7].nodes.size() << '\n';
        std::cout << "size of in_node.nodes[8].nodes: " << in_node.nodes[8].nodes.size() << '\n';
        std::cout << "size of in_node.nodes[9].nodes: " << in_node.nodes[9].nodes.size() << '\n';

        std::cout << '\n';

        std::cout << in_node.nodes[0].step << ' ' << in_node.nodes[0].port << ' ' << in_node.nodes[0].prevNode->port << ' ' << in_node.nodes[0].final << '\n';
        std::cout << in_node.nodes[1].step << ' ' << in_node.nodes[1].port << ' ' << in_node.nodes[1].prevNode->port << ' ' << in_node.nodes[1].final << '\n';
        std::cout << in_node.nodes[2].step << ' ' << in_node.nodes[2].port << ' ' << in_node.nodes[2].prevNode->port << ' ' << in_node.nodes[2].final << '\n';
        std::cout << in_node.nodes[3].step << ' ' << in_node.nodes[3].port << ' ' << in_node.nodes[3].prevNode->port << ' ' << in_node.nodes[3].final << '\n';*/
        //std::cout << &in_node.nodes[0].nodes[0];std::cout << "edc" << '\n';
        //std::cout << in_node.nodes[0].nodes[0].step << ' ' << in_node.nodes[0].nodes[0].port << ' ' << in_node.nodes[0].nodes[0].prevNode->port << ' ' << in_node.nodes[0].nodes[0].final << '\n';
        //std::cout << in_node.nodes[0].nodes[1].step << ' ' << in_node.nodes[0].nodes[1].port << ' ' << in_node.nodes[0].nodes[1].prevNode->port << ' ' << in_node.nodes[0].nodes[1].final << '\n';
    }

public:

    int step{};
    int port{};
    Node* prevNode{};

    bool final{};

    std::list<Node> nodes{};

    Node* currentNode;









};







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

    node.addToNewLevel(0, 17);
    if (node.nodes.empty()) std::cout << "Empty!" << '\n';
    std::cout << "After addToAnotherLevel: " << &node.nodes.front() << '\n';
    node.addToCurrentLevel(0, 16);
    node.addToCurrentLevel(0, 23);
    node.addToCurrentLevel(0, 22);

    //Node::getCurrentNode()->add(2, 113);
    //Node::getCurrentNode()->getPrevNode()->add(2, 112);

    node.addToNewLevel(2, 113);
    if (node.nodes.front().nodes.empty()) std::cout << "Empty!" << '\n';
    std::cout << "After addToAnotherLevel: " << &node.nodes.front().nodes.front() << '\n';
    node.addToCurrentLevel(2, 112);

    node.addToNewLevel(3, 131);
    node.addToCurrentLevel(3, 132);

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
