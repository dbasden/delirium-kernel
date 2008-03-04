/*
 * Display a realm vision.
 *
 * realmid is a hint to the realmid that you want. If it
 * is 0, you are allocated a new one, otherwise, if it
 * still exists, you will get the same space as last time
 * There is no guarantee that you will get the same space
 * as last time. If you supply a realmid other than 0, you
 * must pass the same splats value as when you originally
 * asked for the vision
 *
 * text is the text you want displayed. there must be at least
 * 10 * splats characters in the space pointed to
 *
 * splats is the size of the realm vision. Each vision is
 * made up of 1 or more splats. Each splat is 10 characters.
 * The only placement guarantee you get is that characters in
 * the splats will be next to each other. All splats may be
 * next to each other, or all might be somewhere else entirely.
 * Who knows. If you're lucky splats will be split into rows.
 *
 * The return value is the realmidof the space allocated for
 * future hinting. It can be treated as unique by the caller even
 * after removal from the screen
 */
size_t realm_vision(size_t realmid, char *text, size_t splats) { 
}
