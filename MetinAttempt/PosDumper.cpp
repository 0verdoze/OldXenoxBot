#include "PosDumper.h"
#include "SharedHeader.h"

#include <fstream>
#include <iostream>
#include <Windows.h>

#include "utility.h"

bool called = false;
std::fstream fout;

void dump_pos()
{
    //CALL_EVERY(1000, []() {
    //    auto& chrMgr = *GetObjectPointer<CPythonCharacterManager>();
    //    for (const auto& ply : chrMgr.m_kAliveInstMap)
    //        std::cout << ply.first << ' ' << ply.second->m_stName << '\n';
    //});

    CALL_EVERY(2000, []()
        {
            auto& chrMgr = *GetObjectPointer<CPythonCharacterManager>();
            auto& player = *GetObjectPointer<CPythonPlayer>();

            if (chrMgr.m_pkInstMain == nullptr)
                return;

            const auto& adwEffect = player.m_adwEffect;
            const auto& eftMap = chrMgr.m_pkInstMain->m_kEffectContainer.m_kDct_dwEftID;

            std::cout << "\n\n" << &adwEffect << "\n\n";

            for (size_t i = 0; i < std::size(adwEffect); i++)
                std::cout << i << ' ' << adwEffect[i] << '\n';

            for (const auto& effPair : eftMap)
                std::cout << effPair.first << ": " << effPair.second << '\n';
        });

}
