#include <iostream>

#include <list>
#include <algorithm>


/**
 * Пара ID портов, образующих линк
 */
struct Link
{
    //ID порта №1
    int port1;
    //ID порта №2
    int port2;

    bool operator == (const Link& rhs) const
    {
        if (this->port1 == rhs.port1 && this->port2 == rhs.port2) return true;//TODO сделать проверку на равенство размеров

        return false;
    }
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
 * Поиск линков концентраторов TODO инкапсулировать используемые классы и структуры в этот класс
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
     * @param in_concentratorIDAndMACTableAnalog Контейнер пар ID концентратора и контейнера пар (ID концентратора и ID порта другого
     * концентратора в соответствии с мас-таблицей) в узле дерева поиска линков
     */
    SearchLinks(std::list<ConcentratorIDAndMACTableAnalog>& in_concentratorIDAndMACTableAnalog)
    {
        //Определить текущий узел дерева поиска линков
        currentNode = &node;
        //Инициализировать ID начального концентратора узла дерева поиска линков, в котором перебираются порты, и контейнера пар ID концентратора и
        //контейнера пар (ID концентратора и ID порта другого концентратора в соответствии с мас-таблицей)
        initializePortEnumerationInitialConcentratorIDAndConcentratorIDAndMACTableAnalog(std::move(in_concentratorIDAndMACTableAnalog));
    }

    /**
     * Добавить узел на новый уровень в дерево поиска линков
     * @param in_stepID ID шага поиска линков
     * @param in_concentratorIDPairedWithMACTableAnalog ID концентратора, имеющего этот контейнер пар (соответствующих мас-таблице)
     */
    void addNode(int in_stepID, int in_concentratorIDPairedWithMACTableAnalog, ConcentratorIDAndPortID &in_pairOfConcentratorIDAndPortID)
    {
        //Добавить узел на новый уровень в контейнер узлов дерева поиска линков
        currentNode->nodes.emplace_back<Node>({in_stepID});
        //Инициализировать предыдущий узел в новом узле текущим
        currentNode->nodes.back().previousNode = currentNode;
        //Добавить текущую пару в контейнер пар (ID концентратора и ID порта другого концентратора в соответствии с мас-таблицей)
        // текущего уровня добавленного узла, принадлежащий этому концентратору
        currentNode->nodes.back().concentratorIDAndMACTableAnalog.emplace_back<ConcentratorIDAndMACTableAnalog>(
                {in_concentratorIDPairedWithMACTableAnalog, {in_pairOfConcentratorIDAndPortID}});
    }

    /**
     * Инициализировать ID начального концентратора узла дерева поиска линков, в котором перебираются порты, и контейнера пар ID концентратора и
     * контейнера пар (ID концентратора и ID порта другого концентратора в соответствии с мас-таблицей)
     * @param in_concentratorIDAndMACTableAnalog Контейнер пар ID концентратора и контейнера пар (ID концентратора и ID порта другого
     * концентратора в соответствии с мас-таблицей) в узле дерева поиска линков
     * TODO сделать внешнюю функцию для создания списка
     */
    void initializePortEnumerationInitialConcentratorIDAndConcentratorIDAndMACTableAnalog(std::list<ConcentratorIDAndMACTableAnalog>&& in_concentratorIDAndMACTableAnalog)
    {
        //Инициализировать Контейнер пар ID концентратора и контейнера пар (ID концентратора и ID порта другого
        //концентратора в соответствии с мас-таблицей) в узле дерева поиска линков
        currentNode->concentratorIDAndMACTableAnalog = std::move(in_concentratorIDAndMACTableAnalog);
    }



    /**
     * Определить линки
     * @return
     */
    std::list<Link> run()
    {
        //Пока поиск линков не завершён
        while (!node.final)
        {
            ++count;

            //Если контейнер пар ID концентратора и контейнера пар (ID концентратора и ID порта другого концентратора в соответствии с мас-таблицей)
            //содержтит два элемента (два порта, образующих линк, определены)
            //TODO переделать для проверки наличия конечных пар. Точно ли в этих двух контейнерах осталось по одной паре?
            if (currentNode->concentratorIDAndMACTableAnalog.size() == 2)
            {
                nodeDone();
                --count;
                //Определить ID следующего концентратора имеющего разные порты
                searchPortEnumerationInitialConcentratorID();
                //Перейти к другому узлу
                continue;
            }

            //Определить ID следующего концентратора имеющего разные порты
            searchPortEnumerationInitialConcentratorID();

            //Добавить новые узлы
            addNewNodes();

            //Добавить в новый узел контейнер пар ID концентратора и контейнера пар (ID концентратора и ID порта другого концентратора
            // в соответствии с мас-таблицей) и отбросить лишние ID концентратора и ID порта
            addConcentratorIDAndMACTableAnalogAndRemoveUnnecessaryPorts();

            //Назначить новый узел текущим узлом дерева поиска линков
            currentNode = &currentNode->nodes.front();

            outputConcentratorIDAndMACTableAnalog();
        }

        return links;
    }

    void outputConcentratorIDAndMACTableAnalog(Node& in_node)
    {


        for (auto &elem : in_node.nodes)
        {
            std::cout << "Node: " << nodesCount++ << '\n';

            for (auto &elem2 : elem.concentratorIDAndMACTableAnalog)
            {
                std::cout << elem2.concentratorIDPairedWithMACTableAnalog << '\n';

                for (auto &elem3 : elem2.MACTableAnalogPairedWithConcentrator)
                {
                    std::cout << elem3.concentratorIDPairedWithPort << ' ' << elem3.portIDPairedWithConcentrator << '\n';
                }

                std::cout << '\n';
            }

            outputConcentratorIDAndMACTableAnalog(elem);
        }
    }

    void outputConcentratorIDAndMACTableAnalog()
    {

        if (count == 1)
        {
            std::cout << "nodes 1" << '\n';
            for (auto &nodes1: node.nodes)
            {
                std::cout << "----------" << '\n';
                for (auto &elem2: nodes1.concentratorIDAndMACTableAnalog)
                {
                    std::cout << elem2.concentratorIDPairedWithMACTableAnalog << '\n';

                    for (auto &elem3: elem2.MACTableAnalogPairedWithConcentrator)
                    {
                        std::cout << elem3.concentratorIDPairedWithPort << ' ' << elem3.portIDPairedWithConcentrator
                                  << '\n';
                    }

                    std::cout << '\n';
                }
            }
        }

        if (count == 2)
        {
            std::cout << "nodes 2" << '\n';

            for (auto &nodes2: node.nodes.front().nodes)
            {
                std::cout << "----------" << '\n';
                for (auto &elem2: nodes2.concentratorIDAndMACTableAnalog)
                {
                    std::cout << elem2.concentratorIDPairedWithMACTableAnalog << '\n';

                    for (auto &elem3: elem2.MACTableAnalogPairedWithConcentrator)
                    {
                        std::cout << elem3.concentratorIDPairedWithPort << ' ' << elem3.portIDPairedWithConcentrator
                                  << '\n';
                    }

                    std::cout << '\n';
                }
            }
        }

        if (count == 3)
        {
            std::cout << "nodes 3" << '\n';

            for (auto &nodes2 : node.nodes.front().nodes.front().nodes
                    )
            {
                std::cout << "----------" << '\n';
                for (auto &elem2: nodes2.concentratorIDAndMACTableAnalog)
                {
                    std::cout << elem2.concentratorIDPairedWithMACTableAnalog << '\n';

                    for (auto &elem3: elem2.MACTableAnalogPairedWithConcentrator)
                    {
                        std::cout << elem3.concentratorIDPairedWithPort << ' ' << elem3.portIDPairedWithConcentrator
                                  << '\n';
                    }

                    std::cout << '\n';
                }
            }
        }

        if (count == 4)
        {
            std::cout << "nodes 4" << '\n';

            for (auto &nodes2 : node.nodes.front().nodes.front().nodes.front().nodes)
            {
                std::cout << "----------" << '\n';
                for (auto &elem2: nodes2.concentratorIDAndMACTableAnalog)
                {
                    std::cout << elem2.concentratorIDPairedWithMACTableAnalog << '\n';

                    for (auto &elem3: elem2.MACTableAnalogPairedWithConcentrator)
                    {
                        std::cout << elem3.concentratorIDPairedWithPort << ' ' << elem3.portIDPairedWithConcentrator
                                  << '\n';
                    }

                    std::cout << '\n';
                }
            }
        }

        if (count == 5)
        {
            std::cout << "nodes 5" << '\n';

            for (auto &nodes2 : node.nodes.front().nodes.front().nodes.front().nodes.front().nodes)
            {
                std::cout << "----------" << '\n';
                for (auto &elem2: nodes2.concentratorIDAndMACTableAnalog)
                {
                    std::cout << elem2.concentratorIDPairedWithMACTableAnalog << '\n';

                    for (auto &elem3: elem2.MACTableAnalogPairedWithConcentrator)
                    {
                        std::cout << elem3.concentratorIDPairedWithPort << ' ' << elem3.portIDPairedWithConcentrator
                                  << '\n';
                    }

                    std::cout << '\n';
                }
            }
        }

        if (count == 6)
        {
            std::cout << "nodes 6" << '\n';

            for (auto &nodes2 : node.nodes.front().nodes.front().nodes.front().nodes.front().nodes.back().nodes)
            {
                std::cout << "----------" << '\n';
                for (auto &elem2: nodes2.concentratorIDAndMACTableAnalog)
                {
                    std::cout << elem2.concentratorIDPairedWithMACTableAnalog << '\n';

                    for (auto &elem3: elem2.MACTableAnalogPairedWithConcentrator)
                    {
                        std::cout << elem3.concentratorIDPairedWithPort << ' ' << elem3.portIDPairedWithConcentrator
                                  << '\n';
                    }

                    std::cout << '\n';
                }
            }
        }

        if (count == 7)
        {
            std::cout << "nodes 7" << '\n';

            for (auto &nodes2: node.nodes.front().nodes.back().nodes)
            {
                std::cout << "----------" << '\n';
                for (auto &elem2: nodes2.concentratorIDAndMACTableAnalog)
                {
                    std::cout << elem2.concentratorIDPairedWithMACTableAnalog << '\n';

                    for (auto &elem3: elem2.MACTableAnalogPairedWithConcentrator)
                    {
                        std::cout << elem3.concentratorIDPairedWithPort << ' ' << elem3.portIDPairedWithConcentrator
                                  << '\n';
                    }

                    std::cout << '\n';
                }
            }
        }

        if (count == 8)
        {
            std::cout << "nodes 8" << '\n';
            for (auto &nodes1: (++node.nodes.begin())->nodes)
            {
                std::cout << "----------" << '\n';
                for (auto &elem2: nodes1.concentratorIDAndMACTableAnalog)
                {
                    std::cout << elem2.concentratorIDPairedWithMACTableAnalog << '\n';

                    for (auto &elem3: elem2.MACTableAnalogPairedWithConcentrator)
                    {
                        std::cout << elem3.concentratorIDPairedWithPort << ' ' << elem3.portIDPairedWithConcentrator
                                  << '\n';
                    }

                    std::cout << '\n';
                }
            }
        }


        /*std::cout << "nodes from second" << '\n';

        for (auto &nodes3 : (++node.nodes.begin())->nodes)
        {
            std::cout << "----------" << '\n';
                        for (auto &elem2 : nodes3.concentratorIDAndMACTableAnalog)
            {
                std::cout << elem2.concentratorIDPairedWithMACTableAnalog << '\n';

                for (auto &elem3 : elem2.MACTableAnalogPairedWithConcentrator)
                {
                    std::cout << elem3.concentratorIDPairedWithPort << ' ' << elem3.portIDPairedWithConcentrator << '\n';
                }

                std::cout << '\n';
            }
        }*/
    }

    void output(Node& in_node)
    {
        for (Node &node: in_node.nodes)
        {
            std::cout << node.stepID << ' ' << node.concentratorIDAndMACTableAnalog.front().concentratorIDPairedWithMACTableAnalog << ' ' << node.final << '\n';

            output(node);
        }
    }

    void outputAll(SearchLinks& bypassingPorts)
    {
        std::cout  << '\n' << "Port traversal Tree" << '\n';

        std::cout << bypassingPorts.node.stepID << ' ' << bypassingPorts.node.concentratorIDAndMACTableAnalog.front().concentratorIDPairedWithMACTableAnalog << ' ' << '0' << ' ' << bypassingPorts.node.final << '\n';

        output(bypassingPorts.node);
    }

private:

    //Узел дерева поиска линков начального уровня
    Node node;
    //Текущий узел дерева поиска линков
    Node* currentNode{};
    //ID начального концентратора узла дерева поиска линков, в котором перебираются порты (последовательно оставляется группа портов с одинаковым ID)
    ConcentratorIDAndMACTableAnalog* ptrOfPortEnumerationInitialConcentratorIDAndMACTableAnalog;

    //ID шага поиска линков
    int stepID{};
    //ID текущего порта, имеющего мас-адрес этого концентратора
    //int currentPortIDPairedWithConcentrator{};
    //Контейнер пар ID портов, образующих линк
    std::list<Link> links;



    int nodesCount{};
    int count{};

    /**
     * Добавить в новый узел контейнер пар ID концентратора и контейнера пар (ID концентратора и ID порта другого концентратора в соответствии с мас-таблицей)
     * и отбросить ненужные ID концентратора и ID порта
     */
    void addConcentratorIDAndMACTableAnalogAndRemoveUnnecessaryPorts()
    {
        //Перебрать каждый узел
        for (auto& newNode : currentNode->nodes)
        {
            //Для каждой пары ID концентратора и контейнера пар (ID концентратора и ID порта другого концентратора в соответствии с мас-таблицей)
            //текущего узла дерева поиска линков предыдущего уровня
            //Перебрать каждую пару ConcentratorIDAndMACTableAnalog внешнего узла
            for (auto &pairOfConcentratorIDAndMACTableAnalogCurrentNode : currentNode->concentratorIDAndMACTableAnalog)
            {
                //Для каждого контейнера пар (ID концентратора и ID порта другого концентратора в соответствии с мас-таблицей)
                for (auto &MACTableAnalogOfNewNode: newNode.concentratorIDAndMACTableAnalog.front().MACTableAnalogPairedWithConcentrator)
                {
                    //Если ID концентратора текущего контейнера пар нового узла равно ID концентратора текущей пары ConcentratorIDAndMACTableAnalog текущего узла
                    if (MACTableAnalogOfNewNode.concentratorIDPairedWithPort ==
                        pairOfConcentratorIDAndMACTableAnalogCurrentNode.concentratorIDPairedWithMACTableAnalog)
                    {
                        //Добавить ID концентратора текущей пары ConcentratorIDAndMACTableAnalog текущего узла в новый узел
                        newNode.concentratorIDAndMACTableAnalog.emplace_back<ConcentratorIDAndMACTableAnalog>(
                                {pairOfConcentratorIDAndMACTableAnalogCurrentNode.concentratorIDPairedWithMACTableAnalog,
                                 {}});



                        //Для каждой пары ID концентратора и контейнера пар (ID концентратора и ID порта другого концентратора в соответствии с мас-таблицей) текущего узла
                        for (auto &pairOfConcentratorIDAndPortIDCurrentNode: pairOfConcentratorIDAndMACTableAnalogCurrentNode.MACTableAnalogPairedWithConcentrator)
                        {
                            //Для каждой пары ID концентратора и контейнера пар (ID концентратора и ID порта другого концентратора в соответствии с мас-таблицей) нового узла
                            for (auto &elem: newNode.concentratorIDAndMACTableAnalog.front().MACTableAnalogPairedWithConcentrator)
                            {
                                //Если ID концентратора текущего узла равно ID концентратора нового узла,
                                if (elem.concentratorIDPairedWithPort ==
                                    pairOfConcentratorIDAndPortIDCurrentNode.concentratorIDPairedWithPort ||
                                    //или, если ID концентратора текущего узла равно ID концентратора concentratorIDPairedWithMACTableAnalog нового узла
                                    newNode.concentratorIDAndMACTableAnalog.front().concentratorIDPairedWithMACTableAnalog ==
                                    pairOfConcentratorIDAndPortIDCurrentNode.concentratorIDPairedWithPort)
                                {
                                    //Добавить в новый узел ID концентратора и ID порта текущего узла
                                    newNode.concentratorIDAndMACTableAnalog.back().MACTableAnalogPairedWithConcentrator.emplace_back(
                                            pairOfConcentratorIDAndPortIDCurrentNode);
                                    //Выйти из цикла
                                    break;
                                }
                            }
                        }

                        //Если контейнер MACTableAnalogPairedWithConcentrator нового узла последнего добавленного элемента пуст
                        if (newNode.concentratorIDAndMACTableAnalog.back().MACTableAnalogPairedWithConcentrator.empty())
                        {
                            //Удалить элемент
                            newNode.concentratorIDAndMACTableAnalog.pop_back();
                        }

                        //Выйти из цикла
                        break;
                    }
                }
            }
        }
    }

    /**
     * //Искать порт в контейнере пар (ID концентратора и ID порта другого концентратора в соответствии с мас-таблицей) для
     * определения существующего узла дерева поиска линков, в котором находится искомый порт
     * @param in_currentPortIDPairedWithConcentrator Текущий порт поиска
     * @return Указатель на узел дерева поиска линков
     */
    Node* searchCurrentPortIDPairedWithConcentrator(int in_currentPortIDPairedWithConcentrator)
    {
        //Для каждого узла текущего контейнера узлов дерева поиска линков
        for (auto &in_node : currentNode->nodes)
        {
            //Для каждой пары ID концентратора и контейнера пар (ID концентратора и ID порта другого концентратора в соответствии с мас-таблицей)
            for (auto &pairOfConcentratorIDAndMACTableAnalog : in_node.concentratorIDAndMACTableAnalog)
            {
                //Если искомый порт найден в текщей паре
                if (pairOfConcentratorIDAndMACTableAnalog.MACTableAnalogPairedWithConcentrator.back().portIDPairedWithConcentrator == in_currentPortIDPairedWithConcentrator)
                {
                    //Вернуть адрес узла
                    return &in_node;
                }
            }
        }

        return nullptr;
    }

    /**
     * //Определить ID следующего концентратора имеющего разные порты
     */
    void searchPortEnumerationInitialConcentratorID()
    {
        //Для каждой пары ID концентратора и контейнера пар (ID концентратора и ID порта другого концентратора в соответствии с мас-таблицей)
        //текущего узла дерева поиска линков предыдущего уровня
        for (auto &pairOfConcentratorIDAndMACTableAnalog : currentNode->concentratorIDAndMACTableAnalog)
        {
            //Для каждой пары (ID концентратора и ID порта другого концентратора в соответствии с мас-таблицей), принадлежащей этому концентратору
            for (auto &pairOfConcentratorIDAndPortID : pairOfConcentratorIDAndMACTableAnalog.MACTableAnalogPairedWithConcentrator)
            {
                //Если ID порта последнего элемента не равно текущему ID порта
                if (pairOfConcentratorIDAndMACTableAnalog.MACTableAnalogPairedWithConcentrator.back().portIDPairedWithConcentrator !=
                    pairOfConcentratorIDAndPortID.portIDPairedWithConcentrator)
                {
                    //Назначить ID порта последнего элемента ID начальному концентратору узла дерева поиска линков
                    ptrOfPortEnumerationInitialConcentratorIDAndMACTableAnalog = &pairOfConcentratorIDAndMACTableAnalog;

                    //ID начального концентратора определён
                    return;
                }
            }
        }
    }

    /**
     * Добавить новые узлы
     */
    void addNewNodes()
    {
        //Нового уровня нет
        bool newLevel{};

        //Для каждой пары (ID концентратора и ID порта другого концентратора в соответствии с мас-таблицей), принадлежащей этому концентратору
        for (auto &pairOfConcentratorIDAndPortID: ptrOfPortEnumerationInitialConcentratorIDAndMACTableAnalog->MACTableAnalogPairedWithConcentrator)
        {
            //Если контейнер узлов дерева поиска линков текущего узла (для уровней глубже) пустой
            if (!newLevel)
            {
                //Добавить узел на новый уровень в дерево поиска линков, и текущую пару в контейнер пар (ID концентратора и ID порта другого концентратора в соответствии с мас-таблицей)
                // текущего узла, принадлежащий этому концентратору
                addNode(++stepID,
                        ptrOfPortEnumerationInitialConcentratorIDAndMACTableAnalog->concentratorIDPairedWithMACTableAnalog,
                        pairOfConcentratorIDAndPortID);
                //Новый уровень создан
                newLevel = true;
            }
                //Контейнер узлов дерева поиска линков, в котором находится текущий узел, непустой
            else
            {
                //Искать порт в контейнере пар (ID концентратора и ID порта другого концентратора в соответствии с мас-таблицей)
                //для определения существующего узла дерева поиска линков, в котором находится искомый порт
                Node *nodeOfPortIDPairedWithConcentrator{searchCurrentPortIDPairedWithConcentrator(
                        pairOfConcentratorIDAndPortID.portIDPairedWithConcentrator)};

                //Если порт не найден
                if (!nodeOfPortIDPairedWithConcentrator)
                {
                    //Добавить узел на текущий уровень в дерево поиска линков, и текущую пару в контейнер пар (ID концентратора и ID порта другого концентратора в соответствии с мас-таблицей)
                    // текущего уровня добавленного узла, принадлежащий этому концентратору
                    addNode(++stepID,
                            ptrOfPortEnumerationInitialConcentratorIDAndMACTableAnalog->concentratorIDPairedWithMACTableAnalog,
                            pairOfConcentratorIDAndPortID);
                } else
                    //Если порт найден
                {
                    //Добавить пару ID концентратора и ID порта другого концентратора в соответствии с мас-таблицей
                    nodeOfPortIDPairedWithConcentrator->concentratorIDAndMACTableAnalog.back().MACTableAnalogPairedWithConcentrator.emplace_back(
                            pairOfConcentratorIDAndPortID);
                }
            }
        }
    }

    /**
     *
     * @return
     */
    void nodeDone()
    {
        //Отметить текущий узел сделанным
        currentNode->final = true;

        //Если контейнер пар ID концентратора и контейнера пар (ID концентратора и ID порта другого концентратора в
        //соответствии с мас-таблицей) в узле дерева поиска линков содержит два элемента (два порта образующих линк)
        if (currentNode->concentratorIDAndMACTableAnalog.size() == 2)
        {
            //Добавить в контейнер пар ID портов, образующих линк
            links.emplace_back<Link>(
                    {currentNode->concentratorIDAndMACTableAnalog.begin()->MACTableAnalogPairedWithConcentrator.begin()->portIDPairedWithConcentrator,
                     (++currentNode->concentratorIDAndMACTableAnalog.begin())->MACTableAnalogPairedWithConcentrator.begin()->portIDPairedWithConcentrator});
        }

        //Если текущий узел является корнем
        if (currentNode == &node)
        {
            //Завершить
            return;
        }

        //Получить итератор текущего узла этого уровня
        auto it{currentNode->previousNode->nodes.begin()};
        //Пока не получен итератор текущего узла
        while (&*it != currentNode)
        {
            //Перейти к следующему узлу
            ++it;
        }

        //Если "следующий" узел этого уровня присутствует
        if (++it != currentNode->previousNode->nodes.end())
        {
            //Назначить "следующего" текущим узлом
            currentNode = &*(it);

            //Завершить
            return;
        }
        else
        {
            //Назначить текущим узлом выше по линку
            currentNode = currentNode->previousNode;
            //Рекурсивно определить сделанный концентратор
            nodeDone();
        }
    }
};



struct ConcentratorIDAndPortsID
{
    int concentratorID{};
    std::list<int> portsID;
};

/**
 * Топология
 */
struct Topology
{
    //ID концентратора
    int concentratorID{};
    //Предыдущий узел (концентратор) дерева топологии
    Topology* previousNode{};
    //Предыдущая развилка дерева топологии
    Topology* previousFork{};
    //Обратный линк
    Link reverseLink{};
    //Признак завершения поиска линков в этом узле и во всех подузлах дерева
    bool final{};
    std::list<std::list<int>> concentratorsByBranches;

    //Контейнер узлов дерева поиска линков (рекурсивный)
    std::list<Topology> topologies{};
};

struct TestTopology
{
    int concentratorID;
    int concentratorIDOfNextLevel;
    Link link;
    int previousFork;
    std::list<std::list<int>> concentratorsByBranches;

    bool operator == (const TestTopology& rhs) const
    {
        if (this->concentratorID == rhs.concentratorID &&
            this->concentratorIDOfNextLevel == rhs.concentratorIDOfNextLevel &&
            this->link == rhs.link &&
            this->previousFork == rhs.previousFork &&
            this->concentratorsByBranches == rhs.concentratorsByBranches) return true;//TODO сделать проверку на равенство размеров

        return false;
    }
};

class NetworkTopologyProcessing
{
public:

    NetworkTopologyProcessing() = default;

    NetworkTopologyProcessing(int in_rootConcentrator, const std::list<Link>& in_links) : rootConcentrator{in_rootConcentrator}, tempLinks{in_links}
    {}

    void addRootConcentratorAndLinks(int in_rootConcentrator, const std::list<Link>& in_links)
    {
        rootConcentrator = in_rootConcentrator;
        tempLinks = in_links;

    }

    void addConcentratorIDAndPortsID(std::list<ConcentratorIDAndPortsID> in_concentratorsIdAndPortsId)
    {
        concentratorsIdAndPortsId = in_concentratorsIdAndPortsId;
    }

    void buildTopologyBasedOnLinks()
    {

        topology.concentratorID = rootConcentrator;
        currentConcentrator = &topology;

        //Определить узлы первого уровня и последовательно добавить их. Эти узлы имеют линки к текущему узлу.
        //То есть, надо найти в линках ID его портов, используя таблицу concentratorsIdAndPortsId. После добавления очередного уровня
        //из этой таблицы удалить записи этих концентраторов с портами.

        //Пока построение топологии не завершено
        while (!topology.final)
        {
            ++count;

            if (isLastConcentrator())
            {
                --count;
                concentratorDone();
                continue;
            }

            //Если ID концентратора равно ID концентратора текущего узла
            auto concentratorIDAndPortsID{
                    std::find_if(concentratorsIdAndPortsId.begin(), concentratorsIdAndPortsId.end(),
                                 [this](auto &concentratorIDAndPortsID)
                                 {
                                     return concentratorIDAndPortsID.concentratorID ==
                                            currentConcentrator->concentratorID;
                                 })};

            //Для каждого порта этого концентратора
            for (auto &port: concentratorIDAndPortsID->portsID)
            {
                //Искать в таблице линков эти порты (получить линк текущего концентратора)
                auto itLink{std::find_if(tempLinks.begin(), tempLinks.end(),
                                         [port](Link link)
                                         {
                                             return link.port1 == port || link.port2 == port;
                                         }
                )};

                //Для каждого соответствия ID концентратора и его портов (найти другой концентратор по его порту из полученного линка ранее)
                for (auto &concentratorIDAndPortsID2: concentratorsIdAndPortsId)
                {
                    //Искать в таблице соответствия ID концентратора и его портов порт другого (нового уровня) концентратора
                    if (concentratorIDAndPortsID2.concentratorID != currentConcentrator->concentratorID)
                    {
                        //
                        auto itNewPort{std::find_if(concentratorIDAndPortsID2.portsID.begin(),
                                                    concentratorIDAndPortsID2.portsID.end(),
                                                    [itLink](int port2)
                                                    {
                                                        return port2 == itLink->port1 || port2 == itLink->port2;
                                                    })
                        };

                        if (itNewPort != concentratorIDAndPortsID2.portsID.end())//TODO это условие лишнее?
                        {
                            Link newLink{*itNewPort,
                                         (*itNewPort == itLink->port1 ? itLink->port2 : itLink->port1)};

                            //Добавить концентратор
                            currentConcentrator->topologies.emplace_back<Topology>(
                                    {concentratorIDAndPortsID2.concentratorID, currentConcentrator, {}, newLink,
                                     false, {}, {}});

                            tempLinks.erase(itLink);

                            break;
                        }
                    }
                }
            }

            auto addConcentratorIDToOtherForks{
                    [this](Topology* currentPreviousFork2, int currentConcentratorID, int concentratorID)
                    {
                        //if (currentPreviousFork2)
                        //{
                        while (currentPreviousFork2)
                        {
                            std::list<int> *elementOfConcentratorsByBranches;

                            auto it{currentPreviousFork2->concentratorsByBranches.begin()};
                            while (it != currentPreviousFork2->concentratorsByBranches.end())
                            {
                                auto it2{std::find(it->begin(), it->end(), currentConcentratorID)};
                                if (it2 != it->end())
                                {
                                    it->emplace_back(concentratorID);
                                    break;
                                }
                                ++it;
                            }

                            //elementOfConcentratorsByBranches->emplace_back(concentratorID);

                            currentPreviousFork2 = currentPreviousFork2->previousFork;
                        }
                        //}
                    }
            };

            if (currentConcentrator->topologies.size() > 1)
            {
                //currentPreviousFork = currentConcentrator;

                currentConcentrator->concentratorsByBranches.resize(currentConcentrator->topologies.size());
                auto it{currentConcentrator->concentratorsByBranches.begin()};
                for (auto &concentrator: currentConcentrator->topologies)
                {
                    concentrator.previousFork = currentConcentrator;
                    it->emplace_back(concentrator.concentratorID);
                    addConcentratorIDToOtherForks(currentConcentrator->previousFork, currentConcentrator->concentratorID, concentrator.concentratorID);
                    ++it;
                }


            }
            else
            {
                currentConcentrator->topologies.front().previousFork = currentConcentrator->previousFork;
                //elementOfConcentratorsByBranches->emplace_back(currentConcentrator->topologies.front().concentratorID);
                addConcentratorIDToOtherForks(currentConcentrator->previousFork, currentConcentrator->concentratorID, currentConcentrator->topologies.front().concentratorID);
            }






            //Назначить новый узел текущим узлом дерева поиска линков
            currentConcentrator = &currentConcentrator->topologies.front();
        }

        output();

        std::cout << "Current concentrator: " << currentConcentrator->concentratorID << '\n';
    }

    std::list<TestTopology> testTopology()
    {
        return containerOfTestTopology;
    }

    std::list<Link> definePath(int fromConcentrator, int toConcentrator)
    {
        Topology* currentConcentrator2{searchConcentrator(fromConcentrator)};

        std::cout << currentConcentrator2->concentratorID << " " << currentConcentrator2->reverseLink.port1 << " " << currentConcentrator2->reverseLink.port2;

    }


private:

    Topology topology;

    Topology* currentConcentrator{};

    int rootConcentrator{};

    //Topology* currentPreviousFork{};

    std::list<Link> tempLinks;

    std::list<ConcentratorIDAndPortsID> concentratorsIdAndPortsId;

    std::list<TestTopology> containerOfTestTopology;

    int count{};

    Topology* searchConcentrator(int concentratorID)
    {
        Topology *currentFork{&topology};
        Topology* currentTopology;
        int currentConcentratorID{};

        while (currentConcentratorID != concentratorID)
        {
            auto branch{currentFork->concentratorsByBranches.begin()};
            auto topology2{currentFork->topologies.begin()};
            for (; branch != currentFork->concentratorsByBranches.end(); ++branch, ++topology2)
            {
                if (std::find(branch->begin(), branch->end(), concentratorID) != branch->end())
                {
                    break;
                }
            }

            currentTopology = &*topology2;
            if (currentTopology->concentratorID == concentratorID)
            {
                currentConcentratorID = currentTopology->concentratorID;
                continue;
            }
            while (currentTopology->topologies.size() == 1)
            {
                if (currentTopology->concentratorID == concentratorID)
                {
                    currentConcentratorID = currentTopology->concentratorID;
                    break;
                }
                currentTopology = &currentTopology->topologies.front();
            }
            std::cout << "qqq";
            currentFork = currentTopology;
            currentConcentratorID = currentTopology->concentratorID;
        }

        return currentTopology;
    }

    bool isLastConcentrator()
    {
        //Если ID концентратора равно ID концентратора текущего узла
        auto concentratorIDAndPortsID{
                std::find_if(concentratorsIdAndPortsId.begin(), concentratorsIdAndPortsId.end(),
                             [this](auto &concentratorIDAndPortsID)
                             {
                                 return concentratorIDAndPortsID.concentratorID ==
                                        currentConcentrator->concentratorID;
                             })};

        //Для каждого порта этого концентратора
        for (auto &port: concentratorIDAndPortsID->portsID)
        {
            //Искать в таблице линков эти порты (получить линк текущего концентратора)
            auto itLink{std::find_if(tempLinks.begin(), tempLinks.end(),
                                     [port](Link link)
                                     {
                                         return link.port1 == port || link.port2 == port;
                                     }
            )};

            //У текущего концентратора ещё есть линки (есть концентраторы следующего уровня)
            if (itLink != tempLinks.end())
            {
                return false;
            }
        }

        return true;
    }

    /**
     *
     * @return
     */
    void concentratorDone()
    {
        //Отметить текущий узел сделанным
        currentConcentrator->final = true;

        //Если текущий концентратор является корнем топологии
        if (currentConcentrator == &topology)
        {
            //Завершить
            return;
        }

        //Если текущий узел этого уровня последний
        if (!isLastConcentrator())
        {
            //Завершить
            return;
        }

        //Получить итератор текущего концентратора этого уровня
        auto it{currentConcentrator->previousNode->topologies.begin()};
        //Пока не получен итератор текущего концентратора
        while (&*it != currentConcentrator)
        {
            //Перейти к следующему концентратору
            ++it;
        }

        //Если "следующий" концентратор этого уровня присутствует
        if (++it != currentConcentrator->previousNode->topologies.end())
        {
            //Назначить "следующего" текущим концентратором
            currentConcentrator = &*(it);

            //Завершить
            return;
        }
            //Текущий концентратор этого уровня последний
        else
        {
            //Назначить текущим концентратором выше по линку
            currentConcentrator = currentConcentrator->previousNode;
            //Рекурсивно определить сделанный концентратор
            concentratorDone();
        }
    }

    void output()
    {

        std::cout << topology.concentratorID << ". Concentrators: ";
        for (auto& branch : topology.concentratorsByBranches)
        {
            for (auto concentratorID : branch)
            {
                std::cout << concentratorID << ", ";
            }

            std::cout << " | ";
        }
        std::cout << '\n';
        containerOfTestTopology.emplace_back<TestTopology>({{}, topology.concentratorID, {{}, {}}, {}, topology.concentratorsByBranches});

        for (auto &concentrator: topology.topologies)
        {
            std::cout << concentrator.previousNode->concentratorID << ": " << concentrator.concentratorID << ". Link: "
                      << concentrator.reverseLink.port1 << '-' << concentrator.reverseLink.port2 << ". PrevFork: " << concentrator.previousFork->concentratorID << ". Concentrators: ";
            for (auto& branch : concentrator.concentratorsByBranches)
            {
                for (auto concentratorID : branch)
                {
                    std::cout << concentratorID << ", ";
                }

                std::cout << " | ";
            }

            std::cout << '\n';
            containerOfTestTopology.emplace_back<TestTopology>({concentrator.previousNode->concentratorID, concentrator.concentratorID, {concentrator.reverseLink.port1, concentrator.reverseLink.port2},
                                                                concentrator.previousFork->concentratorID, concentrator.concentratorsByBranches});

        }

        auto it{++topology.topologies.begin()};

        for (auto &concentrator: it->topologies)
        {
            std::cout << concentrator.previousNode->concentratorID << ": " << concentrator.concentratorID << ". Link: "
                      << concentrator.reverseLink.port1 << '-' << concentrator.reverseLink.port2 << ". PrevFork: " << concentrator.previousFork->concentratorID << ". Concentrators: ";
            for (auto& branch : concentrator.concentratorsByBranches)
            {
                for (auto concentratorID : branch)
                {
                    std::cout << concentratorID << ", ";
                }

                std::cout << " | ";
            }

            std::cout << '\n';
            containerOfTestTopology.emplace_back<TestTopology>({concentrator.previousNode->concentratorID, concentrator.concentratorID, {concentrator.reverseLink.port1, concentrator.reverseLink.port2},
                                                                concentrator.previousFork->concentratorID, concentrator.concentratorsByBranches});

        }

        auto it2{topology.topologies.begin()};

        for (auto &concentrator: it2->topologies)
        {
            std::cout << concentrator.previousNode->concentratorID << ": " << concentrator.concentratorID << ". Link: "
                      << concentrator.reverseLink.port1 << '-' << concentrator.reverseLink.port2 << ". PrevFork: " << concentrator.previousFork->concentratorID << ". Concentrators: ";
            for (auto& branch : concentrator.concentratorsByBranches)
            {
                for (auto concentratorID : branch)
                {
                    std::cout << concentratorID << ", ";
                }

                std::cout << " | ";
            }

            std::cout << '\n';
            containerOfTestTopology.emplace_back<TestTopology>({concentrator.previousNode->concentratorID, concentrator.concentratorID, {concentrator.reverseLink.port1, concentrator.reverseLink.port2},
                                                                concentrator.previousFork->concentratorID, concentrator.concentratorsByBranches});

        }

        auto it3{it2->topologies.begin()};

        for (auto &concentrator: it3->topologies)
        {
            std::cout << concentrator.previousNode->concentratorID << ": " << concentrator.concentratorID << ". Link: "
                      << concentrator.reverseLink.port1 << '-' << concentrator.reverseLink.port2 << ". PrevFork: " << concentrator.previousFork->concentratorID << ". Concentrators: ";
            for (auto& branch : concentrator.concentratorsByBranches)
            {
                for (auto concentratorID : branch)
                {
                    std::cout << concentratorID << ", ";
                }

                std::cout << " | ";
            }

            std::cout << '\n';
            containerOfTestTopology.emplace_back<TestTopology>({concentrator.previousNode->concentratorID, concentrator.concentratorID, {concentrator.reverseLink.port1, concentrator.reverseLink.port2},
                                                                concentrator.previousFork->concentratorID, concentrator.concentratorsByBranches});

        }

        auto it4{it3->topologies.begin()};

        for (auto &concentrator: it4->topologies)
        {
            std::cout << concentrator.previousNode->concentratorID << ": " << concentrator.concentratorID << ". Link: "
                      << concentrator.reverseLink.port1 << '-' << concentrator.reverseLink.port2 << ". PrevFork: " << concentrator.previousFork->concentratorID << ". Concentrators: ";
            for (auto& branch : concentrator.concentratorsByBranches)
            {
                for (auto concentratorID : branch)
                {
                    std::cout << concentratorID << ", ";
                }

                std::cout << " | ";
            }

            std::cout << '\n';
            containerOfTestTopology.emplace_back<TestTopology>({concentrator.previousNode->concentratorID, concentrator.concentratorID, {concentrator.reverseLink.port1, concentrator.reverseLink.port2},
                                                                concentrator.previousFork->concentratorID, concentrator.concentratorsByBranches});

        }

        auto it5{it4->topologies.begin()};

        for (auto &concentrator: it5->topologies)
        {
            std::cout << concentrator.previousNode->concentratorID << ": " << concentrator.concentratorID << ". Link: "
                      << concentrator.reverseLink.port1 << '-' << concentrator.reverseLink.port2 << ". PrevFork: " << concentrator.previousFork->concentratorID << ". Concentrators: ";
            for (auto& branch : concentrator.concentratorsByBranches)
            {
                for (auto concentratorID : branch)
                {
                    std::cout << concentratorID << ", ";
                }

                std::cout << " | ";
            }

            std::cout << '\n';
            containerOfTestTopology.emplace_back<TestTopology>({concentrator.previousNode->concentratorID, concentrator.concentratorID, {concentrator.reverseLink.port1, concentrator.reverseLink.port2},
                                                                concentrator.previousFork->concentratorID, concentrator.concentratorsByBranches});

        }

        auto it6{++it5->topologies.begin()};

        for (auto &concentrator: it6->topologies)
        {
            std::cout << concentrator.previousNode->concentratorID << ": " << concentrator.concentratorID << ". Link: "
                      << concentrator.reverseLink.port1 << '-' << concentrator.reverseLink.port2 << ". PrevFork: " << concentrator.previousFork->concentratorID << ". Concentrators: ";
            for (auto& branch : concentrator.concentratorsByBranches)
            {
                for (auto concentratorID : branch)
                {
                    std::cout << concentratorID << ", ";
                }

                std::cout << " | ";
            }

            std::cout << '\n';
            containerOfTestTopology.emplace_back<TestTopology>({concentrator.previousNode->concentratorID, concentrator.concentratorID, {concentrator.reverseLink.port1, concentrator.reverseLink.port2},
                                                                concentrator.previousFork->concentratorID, concentrator.concentratorsByBranches});

        }
        auto it7{it6->topologies.begin()};

        for (auto &concentrator: it7->topologies)
        {
            std::cout << concentrator.previousNode->concentratorID << ": " << concentrator.concentratorID << ". Link: "
                      << concentrator.reverseLink.port1 << '-' << concentrator.reverseLink.port2 << ". PrevFork: " << concentrator.previousFork->concentratorID << ". Concentrators: ";
            for (auto& branch : concentrator.concentratorsByBranches)
            {
                for (auto concentratorID : branch)
                {
                    std::cout << concentratorID << ", ";
                }

                std::cout << " | ";
            }

            std::cout << '\n';
            containerOfTestTopology.emplace_back<TestTopology>({concentrator.previousNode->concentratorID, concentrator.concentratorID, {concentrator.reverseLink.port1, concentrator.reverseLink.port2},
                                                                concentrator.previousFork->concentratorID, concentrator.concentratorsByBranches});

        }
    }



};

int main()
{


    std::list<ConcentratorIDAndMACTableAnalog> concentratorIDAndMACTableAnalog =
            {
                    {
                            {4, {{9, 41}, {62, 41}, {17, 41}, {13, 41}, {8, 41}, {5, 42}, {24, 43}, {6, 41}, {10, 41}, {7, 41}, {2, 41}, {12, 41}, {11, 41}}},
                            {5, {{9, 51}, {62, 51}, {17, 51}, {4, 51}, {11, 51}, {13, 51}, {8, 51}, {12, 51}, {24, 51}, {6, 51}, {10, 51}, {7, 51}, {2, 51}}},
                            {11, {{9, 112}, {62, 112}, {17, 112}, {4, 112}, {13, 113}, {8, 112}, {5, 112}, {6, 112}, {10, 112}, {7, 113}, {2, 112}, {24, 112}, {12, 112}}},
                            {12, {{9, 122}, {62, 122}, {17, 1224}, {4, 122}, {11, 121}, {13, 121}, {8, 122}, {5, 122}, {6, 122}, {10, 122}, {7, 121}, {2, 122}, {24, 122}}},
                            {9, {{62, 91}, {17, 92}, {4, 91}, {11, 92}, {13, 92}, {8, 91}, {12, 92}, {5, 91}, {6, 91}, {10, 91}, {7, 92}, {2, 91}, {24, 91}}},
                            {10, {{9, 102}, {62, 101}, {17, 102}, {4, 101}, {11, 102}, {13, 102}, {8, 101}, {12, 102}, {5, 101}, {24, 101}, {6, 101}, {7, 102}, {2, 101}}},
                            {8, {{9, 82}, {62, 81}, {17, 82}, {4, 81}, {11, 82}, {13, 82}, {12, 82}, {5, 81}, {24, 81}, {6, 81}, {10, 82}, {7, 82}, {2, 81}}},
                            {62, {{9, 17}, {17, 17}, {4, 16}, {11, 17}, {13, 17}, {8, 17}, {12, 17}, {5, 16}, {24, 16}, {6, 23}, {10, 17}, {7, 17}, {2, 22}}},
                            {2, {{9, 21}, {62, 21}, {17, 21}, {4, 21}, {11, 21}, {13, 21}, {8, 21}, {12, 21}, {5, 21}, {24, 21}, {6, 21}, {10, 21}, {7, 21}}},
                            {6, {{9, 61}, {62, 61}, {4, 61}, {11, 61}, {13, 61}, {8, 61}, {12, 61}, {5, 61}, {24, 61}, {10, 61}, {7, 61}, {2, 61}, {17, 61}}},
                            {24, {{9, 241}, {62, 241}, {17, 241}, {4, 241}, {11, 241}, {13, 241}, {8, 241}, {12, 241}, {5, 241}, {6, 241}, {7, 241},{2, 241}, {10, 241}}},
                            {13, {{9, 131}, {62, 131}, {17, 131}, {4, 131}, {8, 131}, {12, 131}, {5, 131}, {24, 131}, {6, 131}, {10, 131}, {7, 132}, {2, 131}, {11, 131}}},
                            {7, {{9, 71}, {62, 71}, {17, 71}, {4, 71}, {13, 71}, {12, 71}, {5, 71}, {24, 71}, {6, 71}, {10, 71}, {2, 71}, {11, 71}, {8, 71}}},
                            {17, {{9, 1748}, {62, 1748}, {4, 1748}, {11, 1748}, {13, 1748}, {8, 1748}, {12, 1748}, {24, 1748}, {6, 1748}, {10, 1748}, {7, 1748}, {2, 1748}, {5, 1748}}}
                    }
            };

    SearchLinks bypassingPorts(concentratorIDAndMACTableAnalog);
    std::list<Link> links{bypassingPorts.run()};

    for (Link &pairOfPorts: links)
    {
        std::cout << pairOfPorts.port1 << ' ' << pairOfPorts.port2 << '\n';

    }



    std::list<Link> originalLinks{{
                                          {102, 91}, {16, 41}, {17, 81}, {23, 61}, {22, 21}, {82, 101}, {92, 122}, {1224, 1748}, {121, 112}, {132, 71}, {131, 113}, {42, 51}, {43, 241}
                                  }};

    if (links == originalLinks) std::cout << "OK!" << '\n';
    else std::cout << "Not OK!" << '\n';





    std::list<ConcentratorIDAndPortsID> concentratorsIDAndPortsID = {
            {
                    {4, {{41, 42, 43}}},
                    {5, {51}},
                    {11, {{112, 113}}},
                    {12, {{122, 1224, 121}}},
                    {9, {{91, 92}}},
                    {10, {{102, 101}}},
                    {8, {{82, 81}}},
                    {62, {{17, 16, 23, 22}}},
                    {2, {21}},
                    {6, {61}},
                    {24, {241}},
                    {13, {{131, 132}}},
                    {7, {71}},
                    {17, {1748}}
            }};

    NetworkTopologyProcessing networkTopologyProcessing(62, originalLinks);
    networkTopologyProcessing.addConcentratorIDAndPortsID(concentratorsIDAndPortsID);
    networkTopologyProcessing.buildTopologyBasedOnLinks();

    std::list<TestTopology> testTopology{
            {
                    {0, 62, {0, 0}, 0, {{8, 10, 9, 12, 17, 11, 13, 7}, {4, 5, 24}, {6}, {2}}},
                    {62, 8, {81, 17}, 62},
                    {62, 4, {41, 16}, 62, {{5}, {24}}},
                    {62, 6, {61, 23}, 62},
                    {62, 2, {21, 22}, 62},
                    {4, 5, {51, 42}, 4},
                    {4, 24, {241, 43}, 4},
                    {8, 10, {101, 82}, 62},
                    {10, 9, {91, 102}, 62},
                    {9, 12, {122, 92}, 62, {{17}, {11, 13, 7}}},
                    {12, 17, {1748, 1224}, 12},
                    {12, 11, {112, 121}, 12},
                    {11, 13, {131, 113}, 12},
                    {13, 7, {71, 132}, 12},

            }
    };

    if (links == originalLinks) std::cout << "OK!" << '\n';
    else std::cout << "Not OK!" << '\n';
    if (networkTopologyProcessing.testTopology() == testTopology) std::cout << "OK!" << '\n';
    else std::cout << "Not OK!" << '\n';

    networkTopologyProcessing.definePath(11, 5);


    return 0;
}
