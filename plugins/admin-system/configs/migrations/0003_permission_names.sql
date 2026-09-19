-- Permissions become readable names in a JSON array ("admin.ban", "*") instead of flag letters.
ALTER TABLE admin_groups ADD COLUMN permissions TEXT NOT NULL DEFAULT '[]';
ALTER TABLE admins ADD COLUMN permissions TEXT NOT NULL DEFAULT '[]';

-- Collect ',"name"' for each letter held, then wrap the list in brackets.
UPDATE admin_groups SET permissions = '';
UPDATE admin_groups SET permissions = CONCAT(permissions, ',"admin.freeze_admins"') WHERE flags LIKE '%a%';
UPDATE admin_groups SET permissions = CONCAT(permissions, ',"admin.hide"') WHERE flags LIKE '%b%';
UPDATE admin_groups SET permissions = CONCAT(permissions, ',"admin.kick"') WHERE flags LIKE '%c%';
UPDATE admin_groups SET permissions = CONCAT(permissions, ',"admin.ban"') WHERE flags LIKE '%d%';
UPDATE admin_groups SET permissions = CONCAT(permissions, ',"admin.unban"') WHERE flags LIKE '%e%';
UPDATE admin_groups SET permissions = CONCAT(permissions, ',"admin.fun"') WHERE flags LIKE '%f%';
UPDATE admin_groups SET permissions = CONCAT(permissions, ',"admin.fun_mode"') WHERE flags LIKE '%g%';
UPDATE admin_groups SET permissions = CONCAT(permissions, ',"admin.health"') WHERE flags LIKE '%h%';
UPDATE admin_groups SET permissions = CONCAT(permissions, ',"admin.bhop"') WHERE flags LIKE '%j%';
UPDATE admin_groups SET permissions = CONCAT(permissions, ',"admin.weapon"') WHERE flags LIKE '%k%';
UPDATE admin_groups SET permissions = CONCAT(permissions, ',"admin.map"') WHERE flags LIKE '%m%';
UPDATE admin_groups SET permissions = CONCAT(permissions, ',"admin.mute"') WHERE flags LIKE '%o%';
UPDATE admin_groups SET permissions = CONCAT(permissions, ',"admin.control"') WHERE flags LIKE '%s%';
UPDATE admin_groups SET permissions = CONCAT(permissions, ',"admin.vote"') WHERE flags LIKE '%v%';
UPDATE admin_groups SET permissions = CONCAT(permissions, ',"admin.wallhack"') WHERE flags LIKE '%w%';
UPDATE admin_groups SET permissions = CONCAT(permissions, ',"*"') WHERE flags LIKE '%z%';
UPDATE admin_groups SET permissions = CONCAT('[', SUBSTR(permissions, 2), ']');

UPDATE admins SET permissions = '';
UPDATE admins SET permissions = CONCAT(permissions, ',"admin.freeze_admins"') WHERE flags LIKE '%a%';
UPDATE admins SET permissions = CONCAT(permissions, ',"admin.hide"') WHERE flags LIKE '%b%';
UPDATE admins SET permissions = CONCAT(permissions, ',"admin.kick"') WHERE flags LIKE '%c%';
UPDATE admins SET permissions = CONCAT(permissions, ',"admin.ban"') WHERE flags LIKE '%d%';
UPDATE admins SET permissions = CONCAT(permissions, ',"admin.unban"') WHERE flags LIKE '%e%';
UPDATE admins SET permissions = CONCAT(permissions, ',"admin.fun"') WHERE flags LIKE '%f%';
UPDATE admins SET permissions = CONCAT(permissions, ',"admin.fun_mode"') WHERE flags LIKE '%g%';
UPDATE admins SET permissions = CONCAT(permissions, ',"admin.health"') WHERE flags LIKE '%h%';
UPDATE admins SET permissions = CONCAT(permissions, ',"admin.bhop"') WHERE flags LIKE '%j%';
UPDATE admins SET permissions = CONCAT(permissions, ',"admin.weapon"') WHERE flags LIKE '%k%';
UPDATE admins SET permissions = CONCAT(permissions, ',"admin.map"') WHERE flags LIKE '%m%';
UPDATE admins SET permissions = CONCAT(permissions, ',"admin.mute"') WHERE flags LIKE '%o%';
UPDATE admins SET permissions = CONCAT(permissions, ',"admin.control"') WHERE flags LIKE '%s%';
UPDATE admins SET permissions = CONCAT(permissions, ',"admin.vote"') WHERE flags LIKE '%v%';
UPDATE admins SET permissions = CONCAT(permissions, ',"admin.wallhack"') WHERE flags LIKE '%w%';
UPDATE admins SET permissions = CONCAT(permissions, ',"*"') WHERE flags LIKE '%z%';
UPDATE admins SET permissions = CONCAT('[', SUBSTR(permissions, 2), ']');

ALTER TABLE admin_groups DROP COLUMN flags;
ALTER TABLE admins DROP COLUMN flags;
