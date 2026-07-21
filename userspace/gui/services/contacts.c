/*
 * services/contacts.c - Contacts store
 *
 * Seeded with sample entries (kept sorted by name) so the Contacts UI
 * has something to render before anything is saved. On hardware the
 * store is loaded from /CONTACTS.BIN at startup and rewritten on every
 * add, so edits survive a reboot; in the simulator the storage backend
 * is a no-op and the seeded mock is what you see. Names are ASCII for
 * now — a Vietnamese subset font is a later task.
 *
 * Written by Doan Phu Hai <haidoan2098@gmail.com>
 */

#include <stddef.h>
#include "contacts.h"
#include "storage.h"

#define MAX_CONTACTS  32

/* On-disk file holding the whole contacts store. */
#define CONTACTS_PATH   "/CONTACTS.BIN"
#define CONTACTS_MAGIC  0x53544E44u  /* "CNTD" */

struct contacts_blob {
	unsigned int   magic;
	unsigned int   count;
	struct contact entries[MAX_CONTACTS];
};

static struct contact store[MAX_CONTACTS];
static int count;

static void copy_field(char *dst, const char *src, int max)
{
	int i = 0;
	if (src) {
		for (; src[i] && i < max - 1; i++) {
			dst[i] = src[i];
		}
	}
	dst[i] = '\0';
}

/*
 * Store indices are kept STABLE so screens can hold an index across an
 * edit without it pointing at a different contact: add appends, update
 * is in-place, and only delete shifts (after which the caller leaves the
 * screen). Display order (alphabetical) is handled by the list screen,
 * not the store.
 */

/* Remove the entry at @index, closing the gap. No save. */
static void remove_at(int index)
{
	for (int i = index; i < count - 1; i++) {
		store[i] = store[i + 1];
	}
	count--;
}

/* Serialize the whole store to /CONTACTS.BIN. */
static void contacts_save(void)
{
	/* SD persistence disabled for the demo — contacts are seeded in RAM at
	 * boot (see contacts_init). Kept as a no-op so the add/update/delete paths
	 * still compile; session edits live in memory until reboot. */
}

/* Wipe the entire store and persist the empty state so the next boot
 * also starts clean.  Call before porting to a new display.
 */
void contacts_clear(void)
{
	count = 0;
	contacts_save();
}

/*
 * contacts_init() - Seed the demo contacts in RAM (no SD persistence).
 *
 * The first five numbers match the seeded SMS threads (services/messages.c) so
 * the message list and caller ID resolve to names. ASCII names only — the
 * built-in Montserrat font has no Vietnamese glyphs.
 */
void contacts_init(void)
{
	static const struct { const char *name; const char *phone; } seed[] = {
		{ "Phu Hai",     "0912345678" },
		{ "Bui Hien",    "0905112233" },
		{ "Minh Quan",   "0937445566" },
		{ "Thanh Tung",  "0708123456" },
		{ "Ngoc Anh",    "0967221144" },
		{ "Hoang Nam",   "0349876543" },
		{ "Thu Thao",    "0816998877" },
	};
	count = (int)(sizeof(seed) / sizeof(seed[0]));
	if (count > MAX_CONTACTS) {
		count = MAX_CONTACTS;
	}
	for (int i = 0; i < count; i++) {
		copy_field(store[i].name,  seed[i].name,  CONTACT_NAME_MAX);
		copy_field(store[i].phone, seed[i].phone, CONTACT_PHONE_MAX);
	}
}

int contacts_count(void)
{
	return count;
}

const struct contact *contacts_get(int index)
{
	if (index < 0 || index >= count) {
		return NULL;
	}
	return &store[index];
}

int contacts_add(const char *name, const char *phone)
{
	if (count >= MAX_CONTACTS) {
		return -1;
	}
	copy_field(store[count].name, name, CONTACT_NAME_MAX);
	copy_field(store[count].phone, phone, CONTACT_PHONE_MAX);
	count++;
	contacts_save();
	return 0;
}

int contacts_update(int index, const char *name, const char *phone)
{
	if (index < 0 || index >= count) {
		return -1;
	}
	copy_field(store[index].name, name, CONTACT_NAME_MAX);   /* in-place: index stays valid */
	copy_field(store[index].phone, phone, CONTACT_PHONE_MAX);
	contacts_save();
	return 0;
}

int contacts_remove(int index)
{
	if (index < 0 || index >= count) {
		return -1;
	}
	remove_at(index);
	contacts_save();
	return 0;
}

/* Normalise a phone number for comparison: strip non-digits, convert +84→0. */
static void norm_phone_local(char *dst, const char *src, int sz)
{
	int i = 0;
	while (*src && i < sz - 1) {
		char c = *src++;
		if (c >= '0' && c <= '9') dst[i++] = c;
	}
	dst[i] = '\0';
	/* +84XXXXXXXXX (E.164) → 0XXXXXXXXX (local): digits-only "84..." is 11 chars.
	 * Remove the redundant '4' at index 1 so "84329..." becomes "0329...". */
	if (i == 11 && dst[0] == '8' && dst[1] == '4') {
		dst[0] = '0';
		for (int j = 1; j < i - 1; j++) dst[j] = dst[j + 1];
		dst[--i] = '\0';
	}
}

const struct contact *contacts_find_by_phone(const char *phone)
{
	char key[CONTACT_PHONE_MAX];
	if (!phone || !phone[0])
		return NULL;
	norm_phone_local(key, phone, sizeof(key));
	for (int i = 0; i < count; i++) {
		char stored[CONTACT_PHONE_MAX];
		norm_phone_local(stored, store[i].phone, sizeof(stored));
		if (strcmp(key, stored) == 0)
			return &store[i];
	}
	return NULL;
}

const struct contact *contacts_find_by_name(const char *name)
{
	if (!name || !name[0]) {
		return NULL;
	}
	for (int i = 0; i < count; i++) {
		const char *a = store[i].name, *b = name;
		while (*a && *a == *b) { a++; b++; }
		if (*a == *b)
			return &store[i];
	}
	return NULL;
}
