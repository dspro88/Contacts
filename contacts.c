#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define NAME_LEN 50
#define PHONE_LEN 20
#define EMAIL_LEN 60
#define DB_FILE "contacts.txt"

typedef struct {
	char name[NAME_LEN];
	char phone[PHONE_LEN];
	char email[EMAIL_LEN];
} Contact;

typedef struct {
	Contact *items;
	size_t count;
	size_t capacity;
} ContactBook;

void book_init(ContactBook *b) {
	b->items = NULL;
	b->count = 0;
	b->capacity = 0;
}

void book_free(ContactBook *b) {
	free(b->items);
	book_init(b);
}

int book_add(ContactBook *b, Contact c) {
	if (b->count == b->capacity) {
		size_t new_cap = b->capacity ? b->capacity * 2 : 4;
		Contact *tmp = realloc(b->items, new_cap * sizeof *tmp);
		if (!tmp) return 0;
		b->items = tmp;
		b->capacity = new_cap;
	}
	b->items[b->count++] = c;
	return 1;
}

void book_remove(ContactBook *b, size_t index) {
	if (index >= b->count) return;
	memmove(&b->items[index], &b->items[index + 1],
			(b->count - index - 1) * sizeof(Contact));
	b->count--;
}

void read_line(const char *prompt, char *buf, size_t size) {
	printf("%s", prompt);
	if(!fgets(buf, (int)size, stdin)) {
		buf[0] = '\0';
		return;
	}
	buf[strcspn(buf, "\n")] = '\0';
	for (char *p = buf; *p; p++) {
		if (*p == '|') *p = ' ';
	}
}

int contains_ci(const char *haystack, const char *needle) {
	size_t hl = strlen(haystack), nl = strlen(needle);
	if (nl == 0) return 1;
	if (nl > hl) return 0;
	for (size_t i = 0; i + nl <= hl; i++) {
		size_t j = 0;
		while (j < nl &&
				tolower((unsigned char)haystack[i + j]) ==
				tolower((unsigned char)needle[j])) {
			j++;
		}
		if (j == nl) return 1;
	}
	return 0;
}

int compare_by_name(const void *a, const void *b) {
	const Contact *ca = a;
	const Contact *cb = b;
	return strcmp(ca->name, cb->name);
}

void print_contact(size_t number, const Contact *c) {
	printf("%2zu. %-20s %-15s %s\n", number, c->name, c->phone, c->email);
}

void action_add(ContactBook *b) {
	Contact c;
	read_line("Name: ", c.name, sizeof c.name);
	if (c.name[0] == '\0') {
		printf("Name cannot be empty.\n");
		return;
	}
	read_line("Phone: ", c.phone, sizeof c.phone);
	read_line("Email: ", c.email, sizeof c.email);

	if (book_add(b, c)) {
		printf("Added %s.\n", c.name);
	} else {
		printf("Out of memory!\n");
	}
}

void action_list (const ContactBook *b) {
	if (b->count == 0) {
		printf("(no contacts yet)\n");
		return;
	}
	for (size_t i =0; i < b->count; i++) {
		print_contact(i + 1, &b->items[i]);
	}
}

void action_search(const ContactBook *b) {
	char query[NAME_LEN];
	read_line("Search for: ", query, sizeof query);

	int found = 0;
	for (size_t i = 0; i < b->count; i++) {
		if (contains_ci(b->items[i].name, query)) {
			print_contact(i + 1, &b->items[i]);
			found = 1;
		}
	}
	if (!found) printf("No matches.\n");
}

void action_delete(ContactBook *b) {
	action_list(b);
	if (b->count == 0) return;

	char buf[16];
	read_line("Number to delete (0 to cancel): ", buf, sizeof buf);
	long n = strtol(buf, NULL, 10);

	if (n < 1 || (size_t)n > b->count) {
		printf("Canceled.\n");
		return;
	}
	printf("Deleted %s.\n", b->items[n - 1].name);
	book_remove(b, (size_t)(n-1));
}

void action_sort(ContactBook *b) {
	qsort(b->items, b->count, sizeof(Contact), compare_by_name);
	printf("Sorted by name.\n");
}

int book_save(const ContactBook *b, const char *path) {
	FILE *f = fopen(path, "w");
	if(!f) return 0;
	for (size_t i = 0; i < b->count; i++) {
		const Contact *c = &b->items[i];
		fprintf(f, "%s|%s|%s\n", c->name, c->phone, c->email);
	}
	fclose(f);
	return 1;
}

void book_load(ContactBook *b, const char *path) {
	FILE *f = fopen(path, "r");
	if (!f) return;

	char line[NAME_LEN + PHONE_LEN + EMAIL_LEN + 8];
	while (fgets(line, sizeof line, f)) {
		Contact c = {"", "", ""};
		int fields = sscanf(line, "%49[^|]|%19[^|]|%59[^]n]", c.name, c.phone, c.email);
		if (fields >=1) book_add(b, c);
	}
	fclose(f);
}

int main(void) {
	ContactBook book;
	book_init(&book);
	book_load(&book, DB_FILE);
	printf("Loaded %zu contact(s) from %s.\n", book.count, DB_FILE);

	char choice[8];
	for(;;) {
		printf("\n=== Contact Book ===\n"
				"1) Add	2) List	3) Search 4) Delete 5) Sort 6) Save & quit\n");
		read_line("Choice: ", choice, sizeof choice);

		switch(choice[0]) {
			case '1': action_add(&book); break;
			case '2': action_list(&book); break;
			case '3': action_search(&book); break;
			case '4': action_delete(&book); break;
			case '5': action_sort(&book); break;
			case '6':
				  if (book_save(&book, DB_FILE)) {
					  printf("Saved %zu contact(s). Bye!\n", book.count);
				  } else {
					  printf("Could not save to %s.\n", DB_FILE);
				  }
				  book_free(&book);
				  return 0;
			default:
				  printf("Unknown option.\n");
		}
	}
}

