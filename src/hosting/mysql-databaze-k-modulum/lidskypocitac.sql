-- phpMyAdmin SQL Dump
-- version 4.9.1
-- https://www.phpmyadmin.net/
--
-- Počítač: sql.endora.cz:3310
-- Vytvořeno: Čtv 19. bře 2020, 12:03
-- Verze serveru: 5.6.45-86.1
-- Verze PHP: 7.3.9

SET SQL_MODE = "NO_AUTO_VALUE_ON_ZERO";
SET AUTOCOMMIT = 0;
START TRANSACTION;
SET time_zone = "+00:00";


/*!40101 SET @OLD_CHARACTER_SET_CLIENT=@@CHARACTER_SET_CLIENT */;
/*!40101 SET @OLD_CHARACTER_SET_RESULTS=@@CHARACTER_SET_RESULTS */;
/*!40101 SET @OLD_COLLATION_CONNECTION=@@COLLATION_CONNECTION */;
/*!40101 SET NAMES utf8mb4 */;

--
-- Databáze: `prihlaseni`
--

-- --------------------------------------------------------

--
-- Struktura tabulky `lidskypocitac`
--

CREATE TABLE `lidskypocitac` (
  `id` int(11) NOT NULL,
  `R1` tinyint(1) NOT NULL,
  `R2` tinyint(1) NOT NULL,
  `R3` tinyint(1) NOT NULL,
  `R4` tinyint(1) NOT NULL,
  `R5` tinyint(1) NOT NULL,
  `R6` tinyint(1) NOT NULL,
  `R7` tinyint(1) NOT NULL,
  `R8` tinyint(1) NOT NULL,
  `casUpravy` timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP
) ENGINE=MyISAM DEFAULT CHARSET=utf8;

--
-- Vypisuji data pro tabulku `lidskypocitac`
--

INSERT INTO `lidskypocitac` (`id`, `R1`, `R2`, `R3`, `R4`, `R5`, `R6`, `R7`, `R8`, `casUpravy`) VALUES
(1, 1, 0, 1, 1, 0, 0, 0, 1, '2020-03-17 11:05:22');

--
-- Klíče pro exportované tabulky
--

--
-- Klíče pro tabulku `lidskypocitac`
--
ALTER TABLE `lidskypocitac`
  ADD PRIMARY KEY (`id`);

--
-- AUTO_INCREMENT pro tabulky
--

--
-- AUTO_INCREMENT pro tabulku `lidskypocitac`
--
ALTER TABLE `lidskypocitac`
  MODIFY `id` int(11) NOT NULL AUTO_INCREMENT, AUTO_INCREMENT=2;
COMMIT;

/*!40101 SET CHARACTER_SET_CLIENT=@OLD_CHARACTER_SET_CLIENT */;
/*!40101 SET CHARACTER_SET_RESULTS=@OLD_CHARACTER_SET_RESULTS */;
/*!40101 SET COLLATION_CONNECTION=@OLD_COLLATION_CONNECTION */;
