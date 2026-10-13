/* RSS feed monitoring (upstream issue #931). */

CREATE TABLE rss_feed (
    id INTEGER PRIMARY KEY,
    url TEXT NOT NULL UNIQUE,
    name TEXT NOT NULL,
    enabled INTEGER NOT NULL DEFAULT 1,
    created_at INTEGER NOT NULL,
    updated_at INTEGER
);

CREATE TABLE rss_feed_filter (
    id INTEGER PRIMARY KEY,
    feed_id INTEGER NOT NULL,
    is_regex INTEGER NOT NULL,
    include_filter TEXT,
    exclude_filter TEXT,

    FOREIGN KEY(feed_id) REFERENCES rss_feed(id) ON DELETE CASCADE
);

/* Items already seen per feed, so a matching torrent is added only once. */
CREATE TABLE rss_item (
    id INTEGER PRIMARY KEY,
    feed_id INTEGER NOT NULL,
    guid TEXT NOT NULL,
    title TEXT,
    added_at INTEGER NOT NULL,

    UNIQUE(feed_id, guid),
    FOREIGN KEY(feed_id) REFERENCES rss_feed(id) ON DELETE CASCADE
);
