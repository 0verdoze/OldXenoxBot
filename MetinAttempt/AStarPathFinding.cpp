#include "AStarPathFinding.h"

#include "AStar.hpp"

#define BOOST_DATE_TIME_NO_LIB
#define BOOST_OVERRIDE
#include <boost/interprocess/managed_windows_shared_memory.hpp>
#include <fstream>
#include <iostream>

using namespace boost::interprocess;

typedef managed_windows_shared_memory::segment_manager segment_manager_t;
typedef allocator<char, segment_manager_t> CharAllocator;
typedef std::vector<char, CharAllocator> CharVector;

constexpr char MAP_SHM_NAME[] = "XENOXMT2KOPALNIA";

constexpr size_t X_SIZE = 335;
constexpr size_t Y_SIZE = 409;

constexpr int MAP_RATIO = 100;

const std::vector<D3DXVECTOR2>* ENTITY_LIST;
AStar::Generator generator;

#define SQUARE(x) ((x) * (x))

bool loaded = false;
void loadMap()
{
    static AStar::Generator gen;

    if (!loaded)
    {
        loaded = true;

        gen.setWorldSize({ X_SIZE, Y_SIZE });
        gen.setHeuristic(AStar::Heuristic::manhattan);
        gen.setDiagonalMovement(true);

        managed_windows_shared_memory sm(open_only, MAP_SHM_NAME);
        auto mapVector = sm.find<CharVector>(MAP_SHM_NAME).first;

        auto cMapIter = mapVector->cbegin();
        for (int x = 0; x < X_SIZE; x++)
        {
            for (int y = 0; y < Y_SIZE; y++)
            {
                if (*cMapIter == 0)
                    gen.addCollision({ x, y });

                ++cMapIter;
            }
        }
    }

    generator = gen;
}

std::vector<D3DXVECTOR2> genPath2(D3DXVECTOR3 origin, D3DXVECTOR3 target, std::vector<D3DXVECTOR3>* entityList)
{
    std::vector<D3DXVECTOR2> ret;
    loadMap(); // generate or restore map file

    for (const auto& entity : *entityList)
    {
        int entity_x = entity.x / MAP_RATIO;
        int entity_y = (-entity.y) / MAP_RATIO;

        // 3x3
        generator.addCollision({ entity_x + 1, entity_y - 1 });
        generator.addCollision({ entity_x + 1, entity_y });
        generator.addCollision({ entity_x + 1, entity_y + 1 });

        generator.addCollision({ entity_x, entity_y - 1 });
        generator.addCollision({ entity_x, entity_y });
        generator.addCollision({ entity_x, entity_y + 1 });

        generator.addCollision({ entity_x - 1, entity_y - 1 });
        generator.addCollision({ entity_x - 1, entity_y });
        generator.addCollision({ entity_x - 1, entity_y + 1 });

        // 2x2
        //generator.addCollision({ (int)entity_x, (int)entity_y });
        //generator.addCollision({ (int)entity_x, (int)ceil(entity_y) });
        //generator.addCollision({ (int)ceil(entity_x), (int)entity_y });
        //generator.addCollision({ (int)ceil(entity_x), (int)ceil(entity_y) });
    }

    generator.removeCollision({ ((int)origin.x) / MAP_RATIO, (-(int)origin.y) / MAP_RATIO });

    auto result = generator.findPath(
        { (int)(origin.x / MAP_RATIO), (int)((-origin.y) / MAP_RATIO) },
        { (int)(target.x / MAP_RATIO), (int)((-target.y) / MAP_RATIO) }
    );

    std::cout << result.size() << '\n';

    if (result.size() == 1)
    {
        if (getDistanceSqrAuto(origin, target) > 100 * 100)
            return { };
    }

    auto begin = result.cbegin();
    auto end = result.cend();

    for (; begin != end; begin++)
    {
        ret.push_back({ (FLOAT)(begin->x * MAP_RATIO), (FLOAT)-(begin->y * MAP_RATIO) });
        std::cout << begin->x << ' ' << -begin->y << '\n';
    }

    delete entityList;

    return ret;
}
