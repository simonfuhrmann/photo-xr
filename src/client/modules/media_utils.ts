import * as stringUtils from './string_utils';
import * as types from './client_types';

function getMediaType(media?: types.SelectedMedia): types.MediaType {
  if (!media) return types.MediaType.UNKNOWN;
  const album = media.album;
  const entry = album.entries[media.index];
  return entry.media;
}

export function isPhotoMedia(media?: types.SelectedMedia) {
  return getMediaType(media) === types.MediaType.IMAGE;
}

export function isVideoMedia(media?: types.SelectedMedia) {
  return getMediaType(media) === types.MediaType.VIDEO;
}

export function isSplatMedia(media?: types.SelectedMedia) {
  return getMediaType(media) === types.MediaType.SPLAT;
}

export function getMediaUrl(media: types.SelectedMedia) {
  const index = media.index;
  const album = media.album;
  const entry = album.entries[index];
  return stringUtils.joinPaths('/media', album.path, entry.name);
}
