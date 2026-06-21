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

/* Number of stored contacts (kept sorted by name). */
int contacts_count(void);

/* Contact at index [0, contacts_count()), or NULL if out of range. */
const struct contact *contacts_get(int index);

/* Append a contact. Returns 0 on success, -1 if the store is full. */
int contacts_add(const char *name, const char *phone);

#endif
