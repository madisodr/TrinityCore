GRANT USAGE ON * . * TO 'hyperion'@'localhost' IDENTIFIED BY 'password' WITH MAX_QUERIES_PER_HOUR 0 MAX_CONNECTIONS_PER_HOUR 0 MAX_UPDATES_PER_HOUR 0 ;

CREATE DATABASE `h_world` DEFAULT CHARACTER SET utf8 COLLATE utf8_general_ci;

CREATE DATABASE `h_characters` DEFAULT CHARACTER SET utf8 COLLATE utf8_general_ci;

CREATE DATABASE `h_auth` DEFAULT CHARACTER SET utf8 COLLATE utf8_general_ci;

CREATE DATABASE `h_hotfixes` DEFAULT CHARACTER SET utf8 COLLATE utf8_general_ci;

GRANT ALL PRIVILEGES ON `h_world` . * TO 'hyperion'@'localhost' WITH GRANT OPTION;

GRANT ALL PRIVILEGES ON `h_characters` . * TO 'hyperion'@'localhost' WITH GRANT OPTION;

GRANT ALL PRIVILEGES ON `h_auth` . * TO 'hyperion'@'localhost' WITH GRANT OPTION;

GRANT ALL PRIVILEGES ON `h_hotfixes` . * TO 'hyperion'@'localhost' WITH GRANT OPTION;
