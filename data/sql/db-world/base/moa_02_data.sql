-- mod-moa: default stage data
-- Re-running this file resets the stage tables to the defaults below.
-- Instances that are not listed here are never gated.

DELETE FROM `moa_instance_unlock`;

INSERT INTO `moa_instance_unlock` (`map_id`, `stage`, `note`) VALUES
-- Stage 1: classic dungeons, Molten Core, Onyxia
(36,  1, 'The Deadmines'),
(33,  1, 'Shadowfang Keep'),
(34,  1, 'The Stockade'),
(389, 1, 'Ragefire Chasm'),
(43,  1, 'Wailing Caverns'),
(48,  1, 'Blackfathom Deeps'),
(47,  1, 'Razorfen Kraul'),
(129, 1, 'Razorfen Downs'),
(90,  1, 'Gnomeregan'),
(209, 1, 'Zul\'Farrak'),
(349, 1, 'Maraudon'),
(70,  1, 'Uldaman'),
(109, 1, 'Sunken Temple'),
(189, 1, 'Scarlet Monastery'),
(230, 1, 'Blackrock Depths'),
(229, 1, 'Blackrock Spire'),
(289, 1, 'Scholomance'),
(329, 1, 'Stratholme'),
(429, 1, 'Dire Maul'),
(409, 1, 'Molten Core'),
(249, 1, 'Onyxia\'s Lair'),
-- Stage 2
(469, 2, 'Blackwing Lair'),
(309, 2, 'Zul\'Gurub'),
-- Stage 3
(509, 3, 'Ruins of Ahn\'Qiraj'),
(531, 3, 'Temple of Ahn\'Qiraj'),
-- Stage 4: TBC dungeons, Karazhan, Gruul, Magtheridon
(543, 4, 'Hellfire Ramparts'),
(542, 4, 'The Blood Furnace'),
(540, 4, 'The Shattered Halls'),
(547, 4, 'The Slave Pens'),
(546, 4, 'The Underbog'),
(545, 4, 'The Steamvault'),
(558, 4, 'Auchenai Crypts'),
(555, 4, 'Shadow Labyrinth'),
(556, 4, 'Sethekk Halls'),
(557, 4, 'Mana-Tombs'),
(554, 4, 'The Mechanar'),
(553, 4, 'The Botanica'),
(552, 4, 'The Arcatraz'),
(560, 4, 'Old Hillsbrad Foothills'),
(269, 4, 'The Black Morass'),
(532, 4, 'Karazhan'),
(565, 4, 'Gruul\'s Lair'),
(544, 4, 'Magtheridon\'s Lair'),
-- Stage 5
(548, 5, 'Serpentshrine Cavern'),
(550, 5, 'The Eye (Tempest Keep)'),
-- Stage 6
(534, 6, 'The Battle for Mount Hyjal'),
(564, 6, 'Black Temple'),
-- Stage 7
(580, 7, 'Sunwell Plateau'),
(585, 7, 'Magisters\' Terrace'),
-- Stage 8: WotLK dungeons, Naxxramas, Sartharion, Malygos, Archavon
(574, 8, 'Utgarde Keep'),
(575, 8, 'Utgarde Pinnacle'),
(576, 8, 'The Nexus'),
(578, 8, 'The Oculus'),
(599, 8, 'Halls of Stone'),
(600, 8, 'Drak\'Tharon Keep'),
(601, 8, 'Azjol-Nerub'),
(602, 8, 'Halls of Lightning'),
(604, 8, 'Gundrak'),
(608, 8, 'The Violet Hold'),
(619, 8, 'Ahn\'kahet: The Old Kingdom'),
(650, 8, 'Trial of the Champion'),
(595, 8, 'The Culling of Stratholme'),
(533, 8, 'Naxxramas'),
(615, 8, 'The Obsidian Sanctum'),
(616, 8, 'The Eye of Eternity'),
(624, 8, 'Vault of Archavon'),
-- Stage 9
(603, 9, 'Ulduar'),
-- Stage 10
(649, 10, 'Trial of the Crusader'),
(632, 10, 'Forge of Souls'),
(658, 10, 'Pit of Saron'),
(668, 10, 'Halls of Reflection'),
-- Stage 11
(631, 11, 'Icecrown Citadel'),
-- Stage 12
(724, 12, 'The Ruby Sanctum');

DELETE FROM `moa_stage_boss`;

INSERT INTO `moa_stage_boss` (`stage`, `boss_entry`, `name`) VALUES
(1,  11502, 'Ragnaros'),
(1,  10184, 'Onyxia'),
(2,  11583, 'Nefarian'),
(2,  14834, 'Hakkar'),
(3,  15727, 'C\'Thun'),
(3,  15339, 'Ossirian the Unscarred'),
(4,  15690, 'Prince Malchezaar'),
(4,  19044, 'Gruul the Dragonkiller'),
(4,  17257, 'Magtheridon'),
(5,  21212, 'Lady Vashj'),
(5,  19622, 'Kael\'thas Sunstrider'),
(6,  17968, 'Archimonde'),
(6,  22917, 'Illidan Stormrage'),
(7,  25315, 'Kil\'jaeden'),
(8,  15990, 'Kel\'Thuzad'),
(8,  28860, 'Sartharion'),
(8,  28859, 'Malygos'),
(9,  33288, 'Yogg-Saron'),
(10, 34564, 'Anub\'arak'),
(11, 36597, 'The Lich King'),
(12, 39863, 'Halion');
