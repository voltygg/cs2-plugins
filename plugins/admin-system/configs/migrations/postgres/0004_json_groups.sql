-- 0004 admins.groups and admin_groups.inherits become JSON-array text

ALTER TABLE admins ALTER COLUMN groups DROP DEFAULT;
ALTER TABLE admins ALTER COLUMN groups TYPE TEXT USING COALESCE(array_to_json(groups)::text, '[]');
ALTER TABLE admins ALTER COLUMN groups SET DEFAULT '[]';
ALTER TABLE admins ALTER COLUMN groups SET NOT NULL;

ALTER TABLE admin_groups ALTER COLUMN inherits DROP DEFAULT;
ALTER TABLE admin_groups ALTER COLUMN inherits TYPE TEXT USING COALESCE(array_to_json(inherits)::text, '[]');
ALTER TABLE admin_groups ALTER COLUMN inherits SET DEFAULT '[]';
ALTER TABLE admin_groups ALTER COLUMN inherits SET NOT NULL;
