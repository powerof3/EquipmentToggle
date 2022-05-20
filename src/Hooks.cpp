#include "Hooks.h"

void Hooks::Install()
{
	Armor::Install();
	logger::info("Installed armor hook");

	Weapon::Install();
	logger::info("Installed weapons hook");

	Head::Install();
	logger::info("Installed head/hair hook");
}
