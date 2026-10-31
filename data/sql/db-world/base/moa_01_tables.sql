-- mod-moa: tables
-- Apply to the world database.

CREATE TABLE IF NOT EXISTS `moa_state` (
  `id`            TINYINT UNSIGNED NOT NULL DEFAULT 1,
  `current_stage` TINYINT UNSIGNED NOT NULL DEFAULT 1,
  `updated_at`    DATETIME NULL DEFAULT NULL,
  PRIMARY KEY (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_general_ci;

-- Which stage unlocks which instance map. Maps not listed here are never gated.
CREATE TABLE IF NOT EXISTS `moa_instance_unlock` (
  `map_id` SMALLINT UNSIGNED NOT NULL,
  `stage`  TINYINT UNSIGNED NOT NULL,
  `note`   VARCHAR(64) NOT NULL DEFAULT '',
  PRIMARY KEY (`map_id`),
  KEY `idx_stage` (`stage`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_general_ci;

-- Stage end bosses. Kill every boss of the current stage to advance.
-- Only the base entry is stored; the module expands difficulty_entry_1..3 by itself.
CREATE TABLE IF NOT EXISTS `moa_stage_boss` (
  `stage`      TINYINT UNSIGNED NOT NULL,
  `boss_entry` INT UNSIGNED NOT NULL,
  `name`       VARCHAR(64) NOT NULL DEFAULT '',
  PRIMARY KEY (`stage`, `boss_entry`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_general_ci;

-- Kill credit for the current stage. Cleared when the stage completes.
CREATE TABLE IF NOT EXISTS `moa_boss_kill` (
  `stage`      TINYINT UNSIGNED NOT NULL,
  `boss_entry` INT UNSIGNED NOT NULL,
  `killed_at`  DATETIME NULL DEFAULT NULL,
  `killed_by`  VARCHAR(32) NOT NULL DEFAULT '',
  PRIMARY KEY (`stage`, `boss_entry`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_general_ci;

INSERT IGNORE INTO `moa_state` (`id`, `current_stage`, `updated_at`)
VALUES (1, 1, NOW());
