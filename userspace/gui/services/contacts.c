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
#define CONTACTS_MAGIC  0x53544E43u  /* "CNTS" */

struct contacts_blob {
	unsigned int   magic;
	unsigned int   count;
	struct contact entries[MAX_CONTACTS];
};

static struct contact store[MAX_CONTACTS] = {
	{ "An Nguyen",  "0912 345 678" },
	{ "Bao Tran",   "0987 654 321" },
	{ "Binh Le",    "0901 111 222" },
	{ "Chau Pham",  "0934 567 890" },
	{ "Dung Vo",    "0978 222 333" },
	{ "Hoa Dang",   "0911 888 999" },
	{ "Khanh Ly",   "0966 123 456" },
	{ "Long Bui",   "0902 555 777" },
	{ "Minh Quan",  "0945 678 123" },
	{ "Nam Hoang",  "0988 333 222" },
	{ "Quang Huy",  "0913 444 555" },
	{ "Trang Do",   "0977 666 111" },
};

static int count = 12;

static void copy_field(char *dst, const char *src, int max)
{
	int i = 0;
	if (src)
		for (; src[i] && i < max - 1; i++)
			dst[i] = src[i];
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
	for (int i = index; i < count - 1; i++)
		store[i] = store[i + 1];
	count--;
}

/* Serialize the whole store to /CONTACTS.BIN. */
static void contacts_save(void)
{
	struct contacts_blob blob;
	blob.magic = CONTACTS_MAGIC;
	blob.count = (unsigned int)count;
	for (int i = 0; i < count; i++)
		blob.entries[i] = store[i];

	storage_write(CONTACTS_PATH, &blob, sizeof(blob));
}

/*
 * contacts_init() - Load persisted contacts, if any.
 *
 * On a valid file the saved entries replace the seeded mock. If the file
 * is missing/short/corrupt (or we are in the simulator) the mock stays.
 */
void contacts_init(void)
{
	struct contacts_blob blob;
	int n = storage_read(CONTACTS_PATH, &blob, sizeof(blob));

	if (n < (int)(sizeof(blob.magic) + sizeof(blob.count)))
		return;
	if (blob.magic != CONTACTS_MAGIC)
		return;
	if (blob.count > MAX_CONTACTS)
		return;

	count = (int)blob.count;
	for (int i = 0; i < count; i++)
		store[i] = blob.entries[i];
}

int contacts_count(void)
{
	return count;
}

const struct contact *contacts_get(int index)
{
	if (index < 0 || index >= count)
		return NULL;
	return &store[index];
}

int contacts_add(const char *name, const char *phone)
{
	if (count >= MAX_CONTACTS)
		return -1;
	copy_field(store[count].name, name, CONTACT_NAME_MAX);
	copy_field(store[count].phone, phone, CONTACT_PHONE_MAX);
	count++;
	contacts_save();
	return 0;
}

int contacts_update(int index, const char *name, const char *phone)
{
	if (index < 0 || index >= count)
		return -1;
	copy_field(store[index].name, name, CONTACT_NAME_MAX);   /* in-place: index stays valid */
	copy_field(store[index].phone, phone, CONTACT_PHONE_MAX);
	contacts_save();
	return 0;
}

int contacts_remove(int index)
{
	if (index < 0 || index >= count)
		return -1;
	remove_at(index);
	contacts_save();
	return 0;
}

const struct contact *contacts_find_by_phone(const char *phone)
{
	if (!phone || !phone[0])
		return NULL;
	for (int i = 0; i < count; i++) {
		const char *a = store[i].phone, *b = phone;
		while (*a && *a == *b) { a++; b++; }
		if (*a == *b) /* both hit '\0' — exact match */
			return &store[i];
	}
	return NULL;
}
