
# My old Metin2 Bot from 2019
This is a bot that i created as my first reverse engineering project
It was used on one of the private servers called XenoxMT2

The main idea behind was to learn basics of reverse engineering
The project contains of bot itself, GuestManager and HostManager

- Bot consisted of `FishBot`, `MineBot` and some others created to accomplish simple goal
- GuestManager, bot was created to work on VMs, GuestManager was supposed to work inside VM
- HostManager was supposed to run on real machine where user could manage all bots, and even respond to private messages

Everything would work like this
- Bots would connect to GuestManager and GuestManagers would connect to HostManager
- HostManager would send commands to GuestManagers and the latter to Bot instances
- All statues and private in-game messages would be forwarded to HostManager via above scheme

The bot consist of multiple applications, but for now only bot, and host gui  is shared here
I will add rest after reviewing it

Since project is old code, its quality might be lacking (organisation, naming convention consistency and comment wise)
