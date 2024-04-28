#include "PathGenerator.h"
#include <array>
#include <functional>
#include <iostream>

struct WayPoint
{
    D3DXVECTOR2 pos;
    WayPoint *previous = nullptr, *next1 = nullptr, *next2 = nullptr;
};

std::vector<D3DXVECTOR2> start
{ {
    {5033.0f,  -8318.f}, // 16384.f
    {7884.0f,  -9336.f}, // 16384.f
    {10527.0f, -9227.f}  // 15999.f
} };

std::vector<D3DXVECTOR2> left
{ {
    {13094.0f,    -8087.67f}, // 15274
    {14382.9f,    -7105.43f}, // 15079.5
    {16786.0f,    -7434.18f}, // 15148.5
    {20089.3f,    -7039.37f}, // 15148.5
    {22000.0f,  -8100.00f},
    {22711.5f,    -12110.3f}, // 15148.5
    {22721.7f,    -14606.7f}, // 15148.5
} };

std::vector<D3DXVECTOR2> right
{ {
    {12191.3f, -11406.9f}, // 15248.5
    {13258.4f, -12311.1f}, // 15092.4
    {15834.5f, -12281.9f}, // 15148.5
    {17342.9f, -13310.1f}, // 15148.5
    {17398.1f, -16975.1f}, // 15148.5
    {15538.9f, -20059.4f}, // 15291
    {13802.9f, -22487.6f}, // 15148.5
    {13817.0f, -25676.9f}, // 15148.5
} };

typedef std::vector<D3DXVECTOR2>::const_iterator VecVec2Iter;

bool generated = false;
WayPoint* startWaypoint = nullptr;

inline void debugPrint3(const char* msg)
{
    return;
    std::cout << msg << '\n';
    Sleep(1000);
}

std::pair<WayPoint*, WayPoint*> appendWaypoints(VecVec2Iter begin, VecVec2Iter end)
{
    WayPoint* last = new WayPoint{ };
    WayPoint* first = last;
    while (begin != end)
    {
        last->next1 = new WayPoint{ *begin, last };
        last = last->next1;
        ++begin;
    }

    WayPoint* ret = first->next1;
    ret->previous = nullptr;

    delete first;
    return { ret, last };
}

void genWayPoints()
{
    debugPrint3("hello there");
    auto result1 = appendWaypoints(std::cbegin(start), std::cend(start));
    startWaypoint = result1.first;

    auto result2 = appendWaypoints(std::cbegin(left), std::cend(left));
    auto result3 = appendWaypoints(std::cbegin(right), std::cend(right));

    result1.second->next1 = result2.first;
    result1.second->next2 = result3.first;
}

bool pathFind(std::deque<const WayPoint*>& currentPath, const WayPoint* target, bool goBack = false)
{
    if (currentPath.back() == target)
        return true;

    if (goBack)
    {
        if (currentPath.back()->previous)
        {
            currentPath.push_back(currentPath.back()->previous);
            if (pathFind(currentPath, target, true))
                return true;
            currentPath.pop_back();
        }

        if (currentPath.back()->next2)
        {
            goto CHECKBOTH;
        }
    }
    else
    {
    CHECKBOTH:
        if (currentPath.back()->next1)
        {
            currentPath.push_back(currentPath.back()->next1);
            if (pathFind(currentPath, target, false))
                return true;
            currentPath.pop_back();
        }

        if (currentPath.back()->next2)
        {
            currentPath.push_back(currentPath.back()->next2);
            if (pathFind(currentPath, target, false))
                return true;
            currentPath.pop_back();
        }
    }

    return false;
}

__declspec(noinline) void checkDistance(const WayPoint* wp, const D3DXVECTOR3& origin, const D3DXVECTOR3& target,
    std::pair<const WayPoint*, double>& closestOrigin, std::pair<const WayPoint*, double>& closestTarget)
{
    // std::cout << wp << '\n';
    double nDOrig = getDistanceSqr(origin, wp->pos);
    double nDTarg = getDistanceSqr(target, wp->pos);

    if (nDOrig < closestOrigin.second)
    {
        closestOrigin.first = wp;
        closestOrigin.second = nDOrig;
    }

    if (nDTarg < closestTarget.second)
    {
        closestTarget.first = wp;
        closestTarget.second = nDTarg;
    }

    if (wp->next1)
    {
        checkDistance(wp->next1, origin, target, closestOrigin, closestTarget);
    }
    if (wp->next2)
    {
        checkDistance(wp->next2, origin, target, closestOrigin, closestTarget);
    }
};

void genInter(decltype(start)& _vec)
{
    decltype(start) vec;

    for (size_t i = 1; i < _vec.size(); i++)
    {
        constexpr int INTERPOINTS = 30;
        const auto& curr = _vec[i];
        const auto& last = _vec[i - 1];

        float step_x = (curr.x - last.x) / INTERPOINTS;
        float step_y = (curr.y - last.y) / INTERPOINTS;

        for (int i = 0; i < INTERPOINTS; i++)
        {
            vec.push_back({ last.x + (step_x * i), last.y + (step_y * i) });
        }
    }

    std::swap(vec, _vec);
}

__declspec(noinline) std::vector<D3DXVECTOR2> genPath(D3DXVECTOR3 origin, D3DXVECTOR3 target, bool* fail)
{
    *fail = false;
    debugPrint3("genWayPoints");
    if (generated == false)
    {
        genInter(start);
        genInter(left);
        genInter(right);

        genWayPoints();
        generated = true;
    }
    debugPrint3("genDone");

    std::pair<const WayPoint*, double>
        closestOrigin{ 0,(std::numeric_limits<double>::max)() },
        closestTarget{ 0,(std::numeric_limits<double>::max)() };

    debugPrint3("fast init done");

    debugPrint3("find close");
    checkDistance(startWaypoint, origin, target, closestOrigin, closestTarget);
    debugPrint3("find close done");

    if (closestOrigin.first == closestTarget.first)
        // && closestOrigin.second + closestTarget.second < getDistanceSqr(origin, target))
    {
        debugPrint3("short eval success");
        return { };
    }

    debugPrint3("t is o");
    if (closestOrigin.first == closestTarget.first)
    {
        debugPrint3("t is o succ");
        return { closestOrigin.first->pos };
    }

    // make path between closestOrigin and closestTarget
    std::deque<const WayPoint*> path;
    path.push_back(closestOrigin.first);

    debugPrint3("Path find begin");

    if (!pathFind(path, closestTarget.first, false))
    {
        if (!pathFind(path, closestTarget.first, true))
        {
            std::cout << "Unable to find path\n";
            *fail = true;
        }
    }

    std::vector<D3DXVECTOR2> retPath;

    auto iter = path.crbegin();
    auto end = path.crend();

    for (; iter != end; iter++)
    {
        std::cout << (*iter)->pos.x << ' ' << (*iter)->pos.y << '\n';
        retPath.push_back((*iter)->pos);
    }

    debugPrint3("Path gen completed");
    return retPath;
};
    