/*
 * services/contacts.c - In-RAM mock contacts store
 *
 * Seeded with sample entries (kept sorted by name) so the Contacts UI
 * has something to render. Names are ASCII for now — a Vietnamese subset
 * font is a later task, see Documentation/02-gui-design.md. Swap this
 * file for a real backend (filesystem / SIM phonebook) without touching
 * any screen code.
 *
 * Written by Doan Phu Hai <haidoan2098@gmail.com>
 */

#include <stddef.h>
#include "contacts.h"

#define MAX_CONTACTS  32

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
	return 0;
}
