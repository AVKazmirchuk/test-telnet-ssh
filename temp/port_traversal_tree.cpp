#include <iostream>

#include <list>


/**
 * Пара ID портов, образующих линк
 */
struct Link
{
    //ID порта №1
    int port1;
    //ID порта №2
    int port2;
};

/**
 * Пара ID концентратора и ID порта другого концентратора в соответствии с мас-таблицей
 */
struct ConcentratorIDAndPortID
{
    //ID концентратора, имеющего мас-адрес на этом порту
    int concentratorIDPairedWithPort;
    //ID порта, имеющего мас-адрес этого концентратора
    int portIDPairedWithConcentrator;
};

/**
 * Пара ID концентратора и контейнера пар (ID концентратора и ID порта другого концентратора в соответствии с мас-таблицей), принадлежащего концентратору
 */
struct ConcentratorIDAndMACTableAnalog
{
    //ID концентратора, имеющего этот контейнер пар (соответствующих мас-таблице)
    int concentratorIDPairedWithMACTableAnalog{};
    //Контейнер пар (ID концентратора и ID порта другого концентратора в соответствии с мас-таблицей), принадлежащий этому концентратору
    std::list<ConcentratorIDAndPortID> MACTableAnalogPairedWithConcentrator;
};

/**
 * Узел дерева поиска линков
 */
struct Node
{
    //ID шага поиска линков
    int stepID{};
    //ID концентратора узла дерева поиска линков, в котором перебираются порты (последовательно оставляется группа портов с одинаковым ID)
    int portEnumerationConcentratorID{};
    //Предыдущий узел дерева поиска линков
    Node* previousNode{};
    //Признак завершения поиска линков в этом узле и во всех подузлах дерева
    bool final{};
    //Контейнер пар ID концентратора и контейнера пар (ID концентратора и ID порта другого концентратора в соответствии с мас-таблицей) в узле дерева поиска линков
    std::list<ConcentratorIDAndMACTableAnalog> concentratorIDAndMACTableAnalog;

    //Контейнер узлов дерева поиска линков (рекурсивный)
    std::list<Node> nodes{};
};


/**
 * Поиск линков концентраторов
 */
class SearchLinks
{

public:

    /**
     * Конструктор по умолчанию
     */
    SearchLinks()
    {
        //Определить текущий узел дерева поиска линков
        currentNode = &node;
    }

    /**
     * Конструктор, принимающий ID начального концентратора, в котором перебираются порты, и контейнер пар
     * ID концентратора, и контейнера пар (ID концентратора и ID порта другого концентратора в соответствии с мас-таблицей)
     * @param portEnumerationInitialConcentratorID ID начального концентратора узла дерева поиска линков, в котором перебираются порты
     * (последовательно оставляется группа портов с одинаковым ID)
     * @param in_concentratorIDAndMACTableAnalog Контейнер пар ID концентратора и контейнера пар (ID концентратора и ID порта другого
     * концентратора в соответствии с мас-таблицей) в узле дерева поиска линков
     */
    SearchLinks(int portEnumerationInitialConcentratorID, std::list<ConcentratorIDAndMACTableAnalog>&& in_concentratorIDAndMACTableAnalog)
    {
        //Определить текущий узел дерева поиска линков
        currentNode = &node;
        //Инициализировать ID начального концентратора узла дерева поиска линков, в котором перебираются порты, и контейнера пар ID концентратора и
        //контейнера пар (ID концентратора и ID порта другого концентратора в соответствии с мас-таблицей)
        initializePortEnumerationInitialConcentratorIDAndConcentratorIDAndMACTableAnalog(portEnumerationInitialConcentratorID, std::move(in_concentratorIDAndMACTableAnalog));
    }

    /**
     * Добавить узел на текущий уровень в дерево поиска линков
     * @param in_stepID ID шага поиска линков
     * @param in_port ID концентратора, имеющего этот контейнер пар (соответствующих мас-таблице)
     */
    void addNodeToCurrentLevel(int in_stepID, int in_concentratorIDPairedWithMACTableAnalog)
    {
        //Назначить временную переменную предыдущего узла дерева поиска линков
        Node* previousNodeTmp{currentNode->previousNode};
        //Добавить узел на текущий уровень в контейнер узлов дерева поиска линков
        currentNode->previousNode->nodes.emplace_back<Node>({in_stepID, in_concentratorIDPairedWithMACTableAnalog});
        //Инициализировать предыдущий узел в новом узле
        currentNode->previousNode->nodes.back().previousNode = previousNodeTmp;
    }

    /**
     * Добавить узел на новый уровень в дерево поиска линков
     * @param in_stepID ID шага поиска линков
     * @param in_concentratorIDPairedWithMACTableAnalog ID концентратора, имеющего этот контейнер пар (соответствующих мас-таблице)
     */
    void addNodeToNewLevel(int in_stepID, int in_concentratorIDPairedWithMACTableAnalog)
    {
        //Добавить узел на новый уровень в контейнер узлов дерева поиска линков
        currentNode->nodes.emplace_back<Node>({in_stepID, in_concentratorIDPairedWithMACTableAnalog});
        //Инициализировать предыдущий узел в новом узле текущим
        currentNode->nodes.front().previousNode = currentNode;
        //Назначить новый узел текущим узлом дерева поиска линков
        currentNode = &currentNode->nodes.front();
    }

    /**
     * Инициализировать ID начального концентратора узла дерева поиска линков, в котором перебираются порты, и контейнера пар ID концентратора и
     * контейнера пар (ID концентратора и ID порта другого концентратора в соответствии с мас-таблицей)
     * @param in_portEnumerationInitialConcentratorID ID начального концентратора узла дерева поиска линков, в котором перебираются порты
     * (последовательно оставляется группа портов с одинаковым ID)
     * @param in_concentratorIDAndMACTableAnalog Контейнер пар ID концентратора и контейнера пар (ID концентратора и ID порта другого
     * концентратора в соответствии с мас-таблицей) в узле дерева поиска линков
     * TODO сделать внешнюю функцию для создания списка
     */
    void initializePortEnumerationInitialConcentratorIDAndConcentratorIDAndMACTableAnalog(int in_portEnumerationInitialConcentratorID, std::list<ConcentratorIDAndMACTableAnalog>&& in_concentratorIDAndMACTableAnalog)
    {
        //Инициализировать ID начального концентратора узла дерева поиска линков, в котором перебираются порты
        portEnumerationInitialConcentratorID = in_portEnumerationInitialConcentratorID;
        //Инициализировать Контейнер пар ID концентратора и контейнера пар (ID концентратора и ID порта другого
        //концентратора в соответствии с мас-таблицей) в узле дерева поиска линков
        currentNode->concentratorIDAndMACTableAnalog = std::move(in_concentratorIDAndMACTableAnalog);
    }

    /**
     * Добавить в новый узел контейнер пар ID концентратора и контейнера пар (ID концентратора и ID порта другого концентратора в соответствии с мас-таблицей)
     * и отбросить ненужные ID концентратора и ID порта
     */
    void addConcentratorIDAndMACTableAnalogAndRemoveUnnecessaryPorts()
    {
        //Для каждой пары ID концентратора и контейнера пар (ID концентратора и ID порта другого концентратора в соответствии с мас-таблицей)
        //узла дерева поиска линков предыдущего уровня
        for (auto& pairOfConcentratorIDAndMACTableAnalog : currentNode->previousNode->concentratorIDAndMACTableAnalog)
        {
            //Если пара ID концентратора и контейнера пар (ID концентратора и ID порта другого концентратора в соответствии с мас-таблицей) найдена
            //(соответствует ID концентратора узла дерева поиска линков, в котором перебираются порты, текущего узла дерева поиска линков)
            if (pairOfConcentratorIDAndMACTableAnalog.concentratorIDPairedWithMACTableAnalog == currentNode->portEnumerationConcentratorID)
            {

                continue;
            } else
            {

                currentNode->concentratorIDAndMACTableAnalog.emplace_back<ConcentratorIDAndMACTableAnalog>({pairOfConcentratorIDAndMACTableAnalog.concentratorIDPairedWithMACTableAnalog, {}});

                for (auto& pairOfConcentratorIDAndPortID : pairOfConcentratorIDAndMACTableAnalog.MACTableAnalogPairedWithConcentrator)
                {
                    if (pairOfConcentratorIDAndPortID.portIDPairedWithConcentrator == currentPortIDPairedWithConcentrator)
                    {
                        continue;
                    } else
                    {
                        currentNode->concentratorIDAndMACTableAnalog.back().MACTableAnalogPairedWithConcentrator.back() = pairOfConcentratorIDAndPortID;
                    }
                }
            }
        }
    }

    /**
     *
     * @return
     */
    std::list<Link> run()
    {
        //Пока поиск линков не завершён
        while (!this->node.final)
        {
            //Для каждой пары ID концентратора и контейнера пар (ID концентратора и ID порта другого концентратора в соответствии с мас-таблицей)
            for (auto &pairOfConcentratorIDAndMACTableAnalog: currentNode->concentratorIDAndMACTableAnalog)
            {
                //Если пара ID концентратора и контейнера пар (ID концентратора и ID порта другого концентратора в соответствии с мас-таблицей) найдена
                if (pairOfConcentratorIDAndMACTableAnalog.concentratorIDPairedWithMACTableAnalog == portEnumerationInitialConcentratorID)
                {
                    //Для каждой пары (ID концентратора и ID порта другого концентратора в соответствии с мас-таблицей), принадлежащей этому концентратору
                    for (auto &pairOfConcentratorIDAndPortID: pairOfConcentratorIDAndMACTableAnalog.MACTableAnalogPairedWithConcentrator)
                    {
                        //Если ID порта, имеющего мас-адрес этого концентратора, равен ID текущего порта
                        if (pairOfConcentratorIDAndPortID.portIDPairedWithConcentrator == currentPortIDPairedWithConcentrator)
                        {
                            //Узел в дереве поиска линков уже добавлен с этим ID порта. Перейти к слудующей паре
                            continue;
                        }

                        //ID текущего порта, имеющего мас-адрес этого концентратора присвоить ID порта
                        currentPortIDPairedWithConcentrator = pairOfConcentratorIDAndPortID.portIDPairedWithConcentrator;

                        //Если предыдущий узел дерева поиска линков отсутствует
                        if (currentNode->previousNode == nullptr)
                        {
                            //Добавить узел на новый уровень в дерево поиска линков
                            addNodeToNewLevel(++stepID,
                                              pairOfConcentratorIDAndMACTableAnalog.concentratorIDPairedWithMACTableAnalog);
                        } else
                        {
                            //Добавить узел на текущий уровень в дерево поиска линков
                            addNodeToCurrentLevel(0,
                                                  pairOfConcentratorIDAndMACTableAnalog.concentratorIDPairedWithMACTableAnalog);
                        }

                        //Добавить в новый узел контейнер пар ID концентратора и контейнера пар (ID концентратора и ID порта другого концентратора
                        // в соответствии с мас-таблицей) и отбросить лишние ID концентратора и ID порта
                        addConcentratorIDAndMACTableAnalogAndRemoveUnnecessaryPorts();

                        //Если контейнер пар ID концентратора и контейнера пар (ID концентратора и ID порта другого концентратора в соответствии с мас-таблицей)
                        //содержтит два элемента (два порта, образующих линк, определены)
                        if (currentNode->concentratorIDAndMACTableAnalog.size() == 2)
                        {

                            nodeDone();
                        }
                    }

                }
                else
                {
                    continue;
                }
            }
        }
    }

    /**
     *
     * @return
     */
    int nodeDone()
    {
        currentNode->final = true;

        links.emplace_back<Link>({currentNode->concentratorIDAndMACTableAnalog.begin()->MACTableAnalogPairedWithConcentrator.begin()->portIDPairedWithConcentrator,
                                  ++currentNode->concentratorIDAndMACTableAnalog.begin()->MACTableAnalogPairedWithConcentrator.begin()->portIDPairedWithConcentrator});

        if (currentNode == &node)
        {
            return currentNode->portEnumerationConcentratorID;
        }

        for (auto it{currentNode->previousNode->nodes.begin()}; it != currentNode->previousNode->nodes.end(); ++it)
        {
            if (currentNode == &*it)
            {
                if (++it != currentNode->previousNode->nodes.end())
                {
                    currentNode = &*(it);
                    if (!currentNode->nodes.empty())
                    {
                        currentNode = &currentNode->nodes.front();
                    }

                } else
                {
                    currentNode = currentNode->previousNode;
                    nodeDone();
                }

                break;
            }
        }

        return currentNode->portEnumerationConcentratorID;
    }



    void output(Node& in_node)
    {
        for (Node &node: in_node.nodes)
        {
            std::cout << node.stepID << ' ' << node.portEnumerationConcentratorID << ' ' << node.previousNode->portEnumerationConcentratorID << ' ' << node.final << '\n';

            output(node);
        }
    }

    void outputAll(SearchLinks& bypassingPorts)
    {
        std::cout  << '\n' << "Port traversal Tree" << '\n';

        std::cout << bypassingPorts.node.stepID << ' ' << bypassingPorts.node.portEnumerationConcentratorID << ' ' << '0' << ' ' << bypassingPorts.node.final << '\n';

        output(bypassingPorts.node);
    }

private:

    //Узел дерева поиска линков начального уровня
    Node node;
    //Текущий узел дерева поиска линков
    Node* currentNode{};
    //ID начального концентратора узла дерева поиска линков, в котором перебираются порты (последовательно оставляется группа портов с одинаковым ID)
    int portEnumerationInitialConcentratorID{};
    //ID шага поиска линков
    int stepID{};
    //ID текущего порта, имеющего мас-адрес этого концентратора
    int currentPortIDPairedWithConcentrator{};
    //Контейнер пар ID портов, образующих линк
    std::list<Link> links;

};



int main()
{
    SearchLinks bypassingPorts;

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
