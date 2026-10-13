/* Settings introduced by upstream PR 1287 (more advanced libtorrent options).
   `value` stays NULL so the effective value is taken from `default_value`
   (Configuration::GetValue selects IFNULL(value, default_value)); without a
   row here, the settings read in session.cpp would resolve to nullopt and
   throw when .value() is called. */

INSERT INTO setting (key, value, default_value)
VALUES
    ('libtorrent.announce_ip',            NULL, '0'),
    ('libtorrent.rate_limit_ip_overhead', NULL, 'true'),
    ('libtorrent.enable_outgoing_utp',    NULL, 'true'),
    ('libtorrent.enable_incoming_utp',    NULL, 'true'),
    ('libtorrent.enable_outgoing_tcp',    NULL, 'true'),
    ('libtorrent.enable_incoming_tcp',    NULL, 'true'),
    ('libtorrent.dht_privacy_lookups',    NULL, 'false');
