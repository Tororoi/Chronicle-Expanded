#pragma once

// Port volumes as the game set them. The mixer plays each at that times its duck, which the game never reads back,
// so its fades step from its own values.
int  PortVolume(int port);
void SetPortVolume(int port, int volume);
// Every port but except plays at gain times the game's volume (1 for as set), until set again.
void DuckPorts(float gain, int except);
