/**
 * Joins multiple path segments, avoiding duplicate slashes.
 * For example, joinPaths('foo/', '/bar', 'baz/') returns 'foo/bar/baz/'.
 */
export function joinPaths(...paths: string[]): string {
  return paths.join('/').replace(/\/+/g, '/');
}
