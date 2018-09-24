-- -*- coding:utf-8; mode:sql; sql-product:sqlite; -*- --

-- Copyright (C) 2018 Karlsruhe Institute of Technology
-- Copyright (C) 2018 Moritz Klammler <moritz.klammler@alumni.kit.edu>
--
-- This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public
-- License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later
-- version.
--
-- This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied
-- warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
-- details.
--
-- You should have received a copy of the GNU General Public License along with this program.  If not, see
-- <http://www.gnu.org/licenses/>.

PRAGMA foreign_keys = ON;

CREATE TABLE IF NOT EXISTS `Graphs` (
    `id`          BLOB_ID         NOT NULL UNIQUE PRIMARY KEY,
    `generator`   ENUM_GENERATORS NOT NULL,
    `file`        TEXT            NOT NULL UNIQUE,
    `nodes`       INTEGER         NOT NULL CHECK (`nodes` >= 0),
    `edges`       INTEGER         NOT NULL CHECK (`edges` >= 0),
    `native`      LOGICAL         DEFAULT 0 CHECK (`native`   IN (0, 1)),
    `poisoned`    LOGICAL         DEFAULT 0 CHECK (`poisoned` IN (0, 1)),
    `seed`        BLOB
);

CREATE TABLE IF NOT EXISTS `Layouts` (
    `id`          BLOB_ID        NOT NULL UNIQUE PRIMARY KEY,
    `graph`       BLOB_ID        NOT NULL REFERENCES `Graphs` ON DELETE CASCADE,
    `layout`      ENUM_LAYOUTS,
    `file`        TEXT           NOT NULL UNIQUE,
    `width`       REAL           CHECK (`width`  ISNULL OR `width`  >= 0.0),
    `height`      REAL           CHECK (`height` ISNULL OR `height` >= 0.0),
    `fingerprint` BLOB           NOT NULL,
    `seed`        BLOB
);

CREATE INDEX IF NOT EXISTS `IndexLayoutsByGraph` ON `Layouts` (`graph`);

CREATE TABLE IF NOT EXISTS `WorseLayouts` (
    `id`     BLOB_ID         NOT NULL UNIQUE REFERENCES `Layouts` ON DELETE CASCADE,
    `parent` BLOB_ID         NOT NULL        REFERENCES `Layouts` ON DELETE CASCADE,
    `method` ENUM_LAY_WORSE  NOT NULL,
    `rate`   REAL            NOT NULL CHECK (`rate` BETWEEN 0.0 AND 1.0)
);

CREATE INDEX IF NOT EXISTS `IndexWorseLayoutsById`     ON `WorseLayouts` (`id`);
CREATE INDEX IF NOT EXISTS `IndexWorseLayoutsByParent` ON `WorseLayouts` (`parent`);

CREATE TABLE IF NOT EXISTS `InterLayouts` (
    `id`        BLOB_ID         NOT NULL UNIQUE REFERENCES `Layouts` ON DELETE CASCADE,
    `parent1st` BLOB_ID         NOT NULL        REFERENCES `Layouts` ON DELETE CASCADE,
    `parent2nd` BLOB_ID         NOT NULL        REFERENCES `Layouts` ON DELETE CASCADE,
    `method`    ENUM_LAY_INTER  NOT NULL,
    `rate`      REAL            NOT NULL CHECK (`rate` BETWEEN 0.0 AND 1.0)
);

CREATE INDEX IF NOT EXISTS `IndexInterLayoutsById`        ON `InterLayouts` (`id`);
CREATE INDEX IF NOT EXISTS `IndexInterLayoutsByParent1st` ON `InterLayouts` (`parent1st`);
CREATE INDEX IF NOT EXISTS `IndexInterLayoutsByParent2nd` ON `InterLayouts` (`parent2nd`);

CREATE TABLE IF NOT EXISTS `PropertiesDisc` (
    `property`          ENUM_PROPERTIES NOT NULL,
    `layout`            BLOB_ID         NOT NULL REFERENCES `Layouts` ON DELETE CASCADE,
    `vicinity`          INTEGER         CHECK (`vicinity` ISNULL OR `vicinity` > 0),
    `size`              INTEGER         NOT NULL CHECK (`size` >= 0),
    `minimum`           REAL            NOT NULL,
    `maximum`           REAL            NOT NULL,
    `mean`              REAL            NOT NULL,
    `rms`               REAL            NOT NULL,
    `entropyIntercept`  REAL            NOT NULL,
    `entropySlope`      REAL            NOT NULL,
    UNIQUE (`property`, `layout`, `vicinity`)
);

CREATE INDEX IF NOT EXISTS `PropertiesDiscByLayout` ON `PropertiesDisc` (`layout`);

CREATE TABLE IF NOT EXISTS `PropertiesCont` (
    `property`          ENUM_PROPERTIES NOT NULL,
    `layout`            BLOB_ID         NOT NULL REFERENCES `Layouts` ON DELETE CASCADE,
    `vicinity`          INTEGER         CHECK (`vicinity` ISNULL OR `vicinity` > 0),
    `size`              INTEGER         NOT NULL CHECK (`size` >= 0),
    `minimum`           REAL            NOT NULL,
    `maximum`           REAL            NOT NULL,
    `mean`              REAL            NOT NULL,
    `rms`               REAL            NOT NULL,
    UNIQUE (`property`, `layout`, `vicinity`)
);

CREATE INDEX IF NOT EXISTS `PropertiesContByLayout` ON `PropertiesCont` (`layout`);

CREATE TABLE IF NOT EXISTS `MajorAxes` (
    `layout`  BLOB_ID NOT NULL PRIMARY KEY REFERENCES `Layouts` ON DELETE CASCADE,
    `x`       REAL    NOT NULL,
    `y`       REAL    NOT NULL,
    CHECK ( (`x` = 0.0 AND `y` = 0.0) OR ABS(`x` * `x` + `y` * `y` - 1.0) <= 1.0E-3 )
);

CREATE TABLE IF NOT EXISTS `MinorAxes` (
    `layout`  BLOB_ID NOT NULL PRIMARY KEY REFERENCES `Layouts` ON DELETE CASCADE,
    `x`       REAL    NOT NULL,
    `y`       REAL    NOT NULL,
    CHECK ( (`x` = 0.0 AND `y` = 0.0) OR ABS(`x` * `x` + `y` * `y` - 1.0) <= 1.0E-3 )
);

CREATE TABLE IF NOT EXISTS `Histograms` (
    `property` ENUM_PROPERTIES NOT NULL,
    `vicinity` INTEGER         CHECK (`vicinity` IS NULL OR `vicinity` > 0),
    `layout`   BLOB_ID         NOT NULL REFERENCES `Layouts` ON DELETE CASCADE,
    `file`     TEXT            NOT NULL UNIQUE,
    `binning`  ENUM_BINNINGS   NOT NULL,
    `bincount` INTEGER         NOT NULL CHECK (`bincount` >  0  ),
    `binwidth` REAL            NOT NULL CHECK (`binwidth` >  0.0),
    `entropy`  REAL            NOT NULL CHECK (`entropy`  >= 0.0),
    UNIQUE (`property`, `layout`, `vicinity`, `binning`, `bincount`)
);

CREATE INDEX IF NOT EXISTS `IndexHistogramsByLayout` ON `Histograms` (`layout`);

CREATE TABLE IF NOT EXISTS `SlidingAverages` (
    `property` ENUM_PROPERTIES NOT NULL,
    `vicinity` INTEGER         CHECK (`vicinity` IS NULL OR `vicinity` > 0),
    `layout`   BLOB_ID         NOT NULL REFERENCES `Layouts` ON DELETE CASCADE,
    `file`     TEXT            NOT NULL UNIQUE,
    `sigma`    REAL            NOT NULL CHECK (`sigma` >  0.0),
    `points`   INTEGER         NOT NULL CHECK (`points` >  0),
    `entropy`  REAL            NOT NULL,  -- differential entropy may be negative
    UNIQUE (`property`, `layout`, `vicinity`)
);

CREATE INDEX IF NOT EXISTS `IndexSlidingAveragesByLayout` ON `SlidingAverages` (`layout`);

CREATE TABLE IF NOT EXISTS `Metrics` (
    `metric`   ENUM_METRICS NOT NULL,
    `layout`   BLOB_ID      NOT NULL REFERENCES `Layouts` ON DELETE CASCADE,
    `value`    REAL,        -- allow NULL because we might have legitimate NaNs
    UNIQUE (`metric`, `layout`)
);

CREATE TABLE IF NOT EXISTS `TestScores` (
    `lhs`   BLOB_ID    REFERENCES `Layouts` ON DELETE SET NULL,
    `rhs`   BLOB_ID    REFERENCES `Layouts` ON DELETE SET NULL,
    `test`  ENUM_TESTS NOT NULL,
    `value` REAL       CHECK (`value` ISNULL OR `value` BETWEEN -1.0 AND +1.0),
    UNIQUE (`lhs`, `rhs`, `test`)
);

CREATE INDEX IF NOT EXISTS `IndexTestScoresByLhs` ON `TestScores` (`lhs`);
CREATE INDEX IF NOT EXISTS `IndexTestScoresByRhs` ON `TestScores` (`rhs`);

CREATE TABLE IF NOT EXISTS `ToolPerformance` (
    `tool` TEXT NOT NULL,
    `time` REAL NOT NULL CHECK (`time` >= 0.0)
);
