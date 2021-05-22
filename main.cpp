#define _CRT_SECURE_NO_WARNINGS

#include <array>
#include <cassert>
#include <csignal>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <fstream>
#include <string>
#include <thread>
#include <vector>

#include "discord.h"

#if defined(_WIN32)
#pragma pack(push, 1)
struct BitmapImageHeader
{
  uint32_t const structSize{sizeof(BitmapImageHeader)};
  int32_t width{0};
  int32_t height{0};
  uint16_t const planes{1};
  uint16_t const bpp{32};
  uint32_t const pad0{0};
  uint32_t const pad1{0};
  uint32_t const hres{2835};
  uint32_t const vres{2835};
  uint32_t const pad4{0};
  uint32_t const pad5{0};

  BitmapImageHeader &operator=(BitmapImageHeader const &) = delete;
};

struct BitmapFileHeader
{
  uint8_t const magic0{'B'};
  uint8_t const magic1{'M'};
  uint32_t size{0};
  uint32_t const pad{0};
  uint32_t const offset{sizeof(BitmapFileHeader) + sizeof(BitmapImageHeader)};

  BitmapFileHeader &operator=(BitmapFileHeader const &) = delete;
};
#pragma pack(pop)
#endif

struct DiscordState
{
  discord::User currentUser;

  std::unique_ptr<discord::Core> core;
};

namespace
{
  volatile bool interrupted{false};
}

void act(DiscordState *state)
{
  // Activity base
  discord::Activity activity{};

  // Current vars
  std::string songname;
  std::string artist;
  // Prev vars
  std::string prevA = "";
  std::string prevS = "";

  // Templates
  // activity.SetDetails("Fruit Tarts");
  // activity.SetState("Pop Snacks");
  // activity.GetAssets().SetSmallImage("applemusic");
  // activity.GetAssets().SetSmallText("i mage");
  // activity.GetAssets().SetLargeText("u mage");
  activity.GetAssets().SetLargeImage("applemusic");
  activity.SetType(discord::ActivityType::Listening);

  // Polling for changes from the file
  while (1)
  {
    std::ifstream MyReadFile("../musicord.txt");
    getline(MyReadFile, songname);
    activity.SetDetails(songname.c_str());
    getline(MyReadFile, artist);
    activity.SetState(artist.c_str());
    if (artist != prevA || songname != prevS)
    {
      prevA = artist;
      prevS = songname;
      state->core->ActivityManager().UpdateActivity(activity, [](discord::Result result) {
        std::cout << ((result == discord::Result::Ok) ? "Succeeded" : "Failed")
                  << " updating activity!\n";
      });
    }
    std::this_thread::sleep_for(std::chrono::seconds(3));
  }
}

int main(int, char **)
{
  DiscordState state{};

  discord::Core *core{};
  auto result = discord::Core::Create(806224932982882324, DiscordCreateFlags_Default, &core);
  state.core.reset(core);
  if (!state.core)
  {
    std::cout << "Failed to instantiate discord core! (err " << static_cast<int>(result) << ")\n";
    std::exit(-1);
  }

  std::thread thread_obj(act, &state);

  std::signal(SIGINT, [](int) { interrupted = true; });

  do
  {
    state.core->RunCallbacks();

    std::this_thread::sleep_for(std::chrono::milliseconds(16));
  } while (!interrupted);

  return 0;
}
