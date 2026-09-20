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

    bool operator == (const Link& rhs) const
    {
        if (this->port1 == rhs.port1 && this->port2 == rhs.port2) return true;

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
    SearchLinks(int portEnumerationInitialConcentratorID, std::list<ConcentratorIDAndMACTableAnalog>& in_concentratorIDAndMACTableAnalog)
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
        //Node* previousNodeTmp{currentNode->previousNode};
        //Добавить узел на текущий уровень в контейнер узлов дерева поиска линков
        currentNode->nodes.emplace_back<Node>({in_stepID, in_concentratorIDPairedWithMACTableAnalog});
        //Инициализировать предыдущий узел в новом узле
        currentNode->nodes.back().previousNode = currentNode;
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
        //currentNode = &currentNode->nodes.front();
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
        //Перебрать каждый узел
        for (auto& newNode : currentNode->nodes)
        {
            //Для каждой пары ID концентратора и контейнера пар (ID концентратора и ID порта другого концентратора в соответствии с мас-таблицей)
            //узла дерева поиска линков предыдущего уровня
            //Перебрать каждую пару ConcentratorIDAndMACTableAnalog внешнего узла
            for (auto &pairOfConcentratorIDAndMACTableAnalogCurrentNode : currentNode->concentratorIDAndMACTableAnalog)
            {
                //Если пара ID концентратора и контейнера пар (ID концентратора и ID порта другого концентратора в соответствии с мас-таблицей) найдена
                //(соответствует с узла дерева поиска линков, в котором перебираются порты, текущего узла дерева поиска линков)
                if (pairOfConcentratorIDAndMACTableAnalogCurrentNode.concentratorIDPairedWithMACTableAnalog == newNode.portEnumerationConcentratorID)//Эта, наверное, лишняя проверка, так как ниже в условии это проверяется
                {
                    //Уже добавленный в функции выше по стеку
                    //continue;
                } else
                {
                    //Признак уже добавленного концентратора
                    bool addedConcentratorID{};
                    //Для каждого контейнера пар (ID концентратора и ID порта другого концентратора в соответствии с мас-таблицей)
                    for (auto &MACTableAnalogOfNewNode : newNode.concentratorIDAndMACTableAnalog.front().MACTableAnalogPairedWithConcentrator)
                    {
                        //Если ID концентратора текущего контейнера пар нового узла равно ID концентратора текущей пары ConcentratorIDAndMACTableAnalog текущего узла
                        if (MACTableAnalogOfNewNode.concentratorIDPairedWithPort == pairOfConcentratorIDAndMACTableAnalogCurrentNode.concentratorIDPairedWithMACTableAnalog)
                        {
                            //Добавить ID концентратора текущей пары ConcentratorIDAndMACTableAnalog текущего узла в новый узел
                            newNode.concentratorIDAndMACTableAnalog.emplace_back<ConcentratorIDAndMACTableAnalog>(
                                    {pairOfConcentratorIDAndMACTableAnalogCurrentNode.concentratorIDPairedWithMACTableAnalog, {}});
                            //Концентратор добавлен
                            addedConcentratorID = true;
                            //Выйти из цикла
                            break;
                        }
                    }

                    //Если концентратор не добавлен
                    if (!addedConcentratorID)
                    {
                        //Перейти к следующей паре ID концентратора и контейнера пар (ID концентратора и ID порта другого концентратора в соответствии с мас-таблицей)
                        continue;
                    }

                    //Для каждой пары ID концентратора и контейнера пар (ID концентратора и ID порта другого концентратора в соответствии с мас-таблицей) текущего узла
                    for (auto &pairOfConcentratorIDAndPortIDCurrentNode : pairOfConcentratorIDAndMACTableAnalogCurrentNode.MACTableAnalogPairedWithConcentrator)
                    {
                        //Для каждой пары ID концентратора и контейнера пар (ID концентратора и ID порта другого концентратора в соответствии с мас-таблицей) нового узла
                        for (auto &elem : newNode.concentratorIDAndMACTableAnalog.front().MACTableAnalogPairedWithConcentrator)
                        {
                            //Если ID концентратора текущего узла равно ID концентратора нового узла,
                            if (pairOfConcentratorIDAndPortIDCurrentNode.concentratorIDPairedWithPort == elem.concentratorIDPairedWithPort ||
                            //или, если ID концентратора текущего узла равно ID концентратора concentratorIDPairedWithMACTableAnalog нового узла
                                pairOfConcentratorIDAndPortIDCurrentNode.concentratorIDPairedWithPort == newNode.concentratorIDAndMACTableAnalog.front().concentratorIDPairedWithMACTableAnalog)
                            {
                                //Добавить в новый узел ID концентратора и ID порта текущего узла
                                newNode.concentratorIDAndMACTableAnalog.back().MACTableAnalogPairedWithConcentrator.emplace_back(
                                        pairOfConcentratorIDAndPortIDCurrentNode);
                                //
                                break;

                            }
                        }
                    }
                }

                //Если контейнер MACTableAnalogPairedWithConcentrator нового узла последнего элемента пуст
                if (newNode.concentratorIDAndMACTableAnalog.back().MACTableAnalogPairedWithConcentrator.empty())
                {
                    //Удалить элемент
                    newNode.concentratorIDAndMACTableAnalog.pop_back();
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

    void searchPortEnumerationInitialConcentratorID()
    {
        for (auto &pairOfConcentratorIDAndMACTableAnalog : currentNode->concentratorIDAndMACTableAnalog)
        {
            //int previousPortID{pairOfConcentratorIDAndMACTableAnalog.MACTableAnalogPairedWithConcentrator.back().portIDPairedWithConcentrator};

            //Для каждой пары (ID концентратора и ID порта другого концентратора в соответствии с мас-таблицей), принадлежащей этому концентратору
            for (auto &pairOfConcentratorIDAndPortID : pairOfConcentratorIDAndMACTableAnalog.MACTableAnalogPairedWithConcentrator)
            {
                if (pairOfConcentratorIDAndMACTableAnalog.MACTableAnalogPairedWithConcentrator.back().portIDPairedWithConcentrator !=
                    pairOfConcentratorIDAndPortID.portIDPairedWithConcentrator)
                {
                    portEnumerationInitialConcentratorID = pairOfConcentratorIDAndMACTableAnalog.concentratorIDPairedWithMACTableAnalog;


                    return;
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
        while (!node.final)
        {
            ++count;
            bool newLevel{};

            /*for (Link &pairOfPorts: links)
            {
                std::cout << pairOfPorts.port1 << ' ' << pairOfPorts.port2 << '\n';

                //output(node);
            }*/

            //output(node);

            std::cout << '\n';

            //Если контейнер пар ID концентратора и контейнера пар (ID концентратора и ID порта другого концентратора в соответствии с мас-таблицей)
            //содержтит два элемента (два порта, образующих линк, определены)
            if (currentNode->concentratorIDAndMACTableAnalog.size() == 2)//TODO переделать для проверки наличия конечных пар. Точно ли в этих двух контейнерах осталось по одной паре?
            {
                nodeDone();
                --count;
                searchPortEnumerationInitialConcentratorID();
                continue;
            }


                std::cout << "portEnumerationInitialConcentratorID: " << portEnumerationInitialConcentratorID << '\n';
            if (count == 6) std::cout << "count6" << '\n';

                //Для каждой пары ID концентратора и контейнера пар (ID концентратора и ID порта другого концентратора в соответствии с мас-таблицей)
                for (auto &pairOfConcentratorIDAndMACTableAnalog : currentNode->concentratorIDAndMACTableAnalog)
                {


                    //Если пара ID концентратора и контейнера пар (ID концентратора и ID порта другого концентратора в соответствии с мас-таблицей) найдена
                    //(соответствует ID начального концентратора узла дерева поиска линков)
                    if (pairOfConcentratorIDAndMACTableAnalog.concentratorIDPairedWithMACTableAnalog ==
                        portEnumerationInitialConcentratorID)
                    {
                        if (count == 6) std::cout << "count6" << '\n';
                        //Для каждой пары (ID концентратора и ID порта другого концентратора в соответствии с мас-таблицей), принадлежащей этому концентратору
                        for (auto &pairOfConcentratorIDAndPortID : pairOfConcentratorIDAndMACTableAnalog.MACTableAnalogPairedWithConcentrator)
                        {

                            //Если контейнер узлов дерева поиска линков текущего узла (для уровней глубже) пустой
                            if (!newLevel)
                            {
                                if (count == 6) std::cout << "count6" << '\n';
                                //std::cout << "addNodeToNewLevel" << '\n';
                                //Добавить узел на новый уровень в дерево поиска линков
                                addNodeToNewLevel(++stepID,
                                                  pairOfConcentratorIDAndMACTableAnalog.concentratorIDPairedWithMACTableAnalog);
                                //std::cout << pairOfConcentratorIDAndMACTableAnalog.concentratorIDPairedWithMACTableAnalog << '\n';
                                //Добавить текущую пару в контейнер пар (ID концентратора и ID порта другого концентратора в соответствии с мас-таблицей)
                                // текущего узла, принадлежащий этому концентратору
                                currentNode->nodes.front().concentratorIDAndMACTableAnalog.emplace_back<ConcentratorIDAndMACTableAnalog>(
                                        {pairOfConcentratorIDAndMACTableAnalog.concentratorIDPairedWithMACTableAnalog, {pairOfConcentratorIDAndPortID}});
                                //std::cout << currentNode->concentratorIDAndMACTableAnalog.front().concentratorIDPairedWithMACTableAnalog << '\n' <<
                                //currentNode->concentratorIDAndMACTableAnalog.front().MACTableAnalogPairedWithConcentrator.front().concentratorIDPairedWithPort << ' ' <<
                                //currentNode->concentratorIDAndMACTableAnalog.front().MACTableAnalogPairedWithConcentrator.front().portIDPairedWithConcentrator << '\n';
                                newLevel = true;
                                /*outputConcentratorIDAndMACTableAnalog();
                                std::cout << "Press any key..." << '\n';
                                getchar();//*/
                            }
                                //Контейнер узлов дерева поиска линков, в котором находится текущий узел, непустой
                            else
                            {
                                if (count == 6) std::cout << "count6" << '\n';
                                //std::cout << "addNodeToCurrentLevel" << '\n';
                                //Искать порт в контейнере пар (ID концентратора и ID порта другого концентратора в соответствии с мас-таблицей)
                                //для определения существующего узла дерева поиска линков, в котором находится искомый порт
                                Node *nodeOfPortIDPairedWithConcentrator{searchCurrentPortIDPairedWithConcentrator(
                                        pairOfConcentratorIDAndPortID.portIDPairedWithConcentrator)};

                                //Если порт найден
                                if (nodeOfPortIDPairedWithConcentrator)
                                {
                                    //std::cout << "portIDPairedWithConcentrator exists" << '\n';
                                    //Добавить пару ID концентратора и ID порта другого концентратора в соответствии с мас-таблицей
                                    nodeOfPortIDPairedWithConcentrator->concentratorIDAndMACTableAnalog.back().MACTableAnalogPairedWithConcentrator.emplace_back(
                                            pairOfConcentratorIDAndPortID);
                                }
                                //Если порт не найден
                                else
                                {
                                    //std::cout << "portIDPairedWithConcentrator not exists" << '\n';
                                    //Добавить узел на текущий уровень в дерево поиска линков
                                    addNodeToCurrentLevel(0,
                                                          pairOfConcentratorIDAndMACTableAnalog.concentratorIDPairedWithMACTableAnalog);



                                    //Добавить текущую пару в контейнер пар (ID концентратора и ID порта другого концентратора в соответствии с мас-таблицей)
                                    // текущего уровня добавленного узла, принадлежащий этому концентратору
                                    currentNode->nodes.back().concentratorIDAndMACTableAnalog.emplace_back<ConcentratorIDAndMACTableAnalog>(
                                            {pairOfConcentratorIDAndMACTableAnalog.concentratorIDPairedWithMACTableAnalog, {pairOfConcentratorIDAndPortID}});


                                }
                                /*outputConcentratorIDAndMACTableAnalog();
                                std::cout << "Press any key..." << '\n';
                                getchar();//*/
                            }


                        }
                        //outputConcentratorIDAndMACTableAnalog();
                        //Перебор портов текущего узла закончен
                        break;

                    }

                }
                /*outputConcentratorIDAndMACTableAnalog();
                std::cout << "before addConcentratorIDAndMACTableAnalogAndRemoveUnnecessaryPorts" << '\n';
                getchar();//*/
                //Добавить в новый узел контейнер пар ID концентратора и контейнера пар (ID концентратора и ID порта другого концентратора
                // в соответствии с мас-таблицей) и отбросить лишние ID концентратора и ID порта
                addConcentratorIDAndMACTableAnalogAndRemoveUnnecessaryPorts();


                //Назначить новый узел текущим узлом дерева поиска линков
                currentNode = &currentNode->nodes.front();//*/


            searchPortEnumerationInitialConcentratorID();



            //outputConcentratorIDAndMACTableAnalog();

            /*newLevel = false;
            //Назначить новый узел текущим узлом дерева поиска линков
            currentNode = &currentNode->nodes.front();//*/

            //system("cls");
            outputConcentratorIDAndMACTableAnalog();
            std::cout << "1 while is done" << '\n';
            getchar();//*/
        }

        return links;
    }


    /**
     *
     * @return
     */
    int nodeDone()
    {
        std::cout << "nodeDone" << '\n';
        std::cout << currentNode->portEnumerationConcentratorID << '\n';
        std::cout << currentNode->concentratorIDAndMACTableAnalog.front().MACTableAnalogPairedWithConcentrator.front().portIDPairedWithConcentrator << "\n";

        currentNode->final = true;

        if (currentNode->concentratorIDAndMACTableAnalog.size() == 2)
        {
            links.emplace_back<Link>(
                    {currentNode->concentratorIDAndMACTableAnalog.begin()->MACTableAnalogPairedWithConcentrator.begin()->portIDPairedWithConcentrator,
                     (++currentNode->concentratorIDAndMACTableAnalog.begin())->MACTableAnalogPairedWithConcentrator.begin()->portIDPairedWithConcentrator});
        }

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

                    std::cout << "portIDPairedWithConcentrator: " <<  currentNode->concentratorIDAndMACTableAnalog.front().MACTableAnalogPairedWithConcentrator.front().portIDPairedWithConcentrator << '\n';
                    std::cout << "portIDPairedWithConcentrator: " <<  currentNode->concentratorIDAndMACTableAnalog.back().MACTableAnalogPairedWithConcentrator.back().portIDPairedWithConcentrator << '\n';

                    return currentNode->portEnumerationConcentratorID;

                    /*if (!currentNode->nodes.empty())
                    {
                        currentNode = &currentNode->nodes.front();
                    }*/

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
            std::cout << node.stepID << ' ' << node.portEnumerationConcentratorID << ' ' << node.final << '\n';

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
    //int currentPortIDPairedWithConcentrator{};
    //Контейнер пар ID портов, образующих линк
    std::list<Link> links;



    int nodesCount{};
    int count{};
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

    SearchLinks bypassingPorts(62, concentratorIDAndMACTableAnalog);
    std::list<Link> links{bypassingPorts.run()};

    for (Link &pairOfPorts: links)
    {
        std::cout << pairOfPorts.port1 << ' ' << pairOfPorts.port2 << '\n';

    }

    /*bypassingPorts.addToNewLevel(0, 17);
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
    //bypassingPorts.nodeDone();*/

    std::list<Link> originalLinks{{
                                          {102, 91}, {81, 17}, {82, 101}, {92, 122}, {1224, 1748}, {121, 112}, {132, 71}, {131, 113}, {41, 16}, {42, 51}, {43, 241}, {23, 61}, {22, 21}
    }};

    if (links == originalLinks) std::cout << "OK!" << '\n';
    else std::cout << "Not OK!" << '\n';

    return 0;
}
