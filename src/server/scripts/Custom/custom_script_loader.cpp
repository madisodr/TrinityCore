/*
 * Copyright (C) 2008-2019 TrinityCore <https://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program. If not, see <http://www.gnu.org/licenses/>.
 */

// This is where scripts' loading functions should be declared:
void AddSC_Ares();
void AddSC_Athena();
void AddSC_Generator();
void AddSC_Grimoire();
void AddSC_Helios();
void AddSC_Janus();
void AddSC_OOCChat();
void AddSC_Pegasus();
void AddSC_Prometheus();

void AddCustomScripts() {
	AddSC_Ares();
    AddSC_Athena();
    AddSC_Generator();
    AddSC_Grimoire();
	AddSC_Helios();
	AddSC_Janus();
	AddSC_OOCChat();
	AddSC_Pegasus();
	AddSC_Prometheus();
}
