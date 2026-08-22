#pragma once

class Game;

// Install Tomba! 2's game/engine overrides into psxport's shared dispatch registry.
void register_engine_overrides(Game &game);
