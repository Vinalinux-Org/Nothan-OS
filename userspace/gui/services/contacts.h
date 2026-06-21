#ifndef __GUI_CONTACTS_H
#define __GUI_CONTACTS_H

/*
 * Contacts store — the data layer behind the Contacts/Phone/SMS apps.
 *
 * The UI talks only to this interface, never to where contacts actually
 * live. Today the backend is an in-RAM mock seeded with sample entries;
 * when a writable filesystem (or the SIM phonebook) lands, only this
 * file changes and the screens stay as they are.
 */

#define CONTACT_NAME_MAX   32
#define CONTACT_PHONE_MAX  24

struct contact {
	char name[CONTACT_NAME_MAX];
	char phone[CONTACT_PHONE_MAX];
};

/* Load persisted contacts at startup; falls back to the seeded mock. */
void contacts_init(void);

/* Number of stored contacts (kept sorted by name). */
int contacts_count(void);

/* Contact at index [0, contacts_count()), or NULL if out of range. */
const struct contact *contacts_get(int index);

/* Insert a contact, kept sorted by name. Returns 0, or -1 if full. */
int contacts_add(const char *name, const char *phone);

/* Replace the contact at @index (re-sorts). Returns 0, -1 on bad index. */
int contacts_update(int index, const char *name, const char *phone);

/* Delete the contact at @index. Returns 0, -1 on bad index. */
int contacts_remove(int index);

/* Find a contact by exact phone string, or NULL. For Phone/SMS to show
 * a saved name instead of a raw number. */
const struct contact *contacts_find_by_phone(const char *phone);

#endif
