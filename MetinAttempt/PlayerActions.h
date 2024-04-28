#pragma once


typedef DWORD CurrentAction;

constexpr CurrentAction MINING_ACTION = 0x01'01'45'00;
constexpr CurrentAction IDLE_ACTION = 0x01'00'01'00;

constexpr CurrentAction FISHING_IDLE = 0x08'00'01'00;
constexpr CurrentAction FISHING_MOVING = 0x08'00'03'00;
constexpr CurrentAction FISHING_ACTION = 0x08'00'1b'00;
