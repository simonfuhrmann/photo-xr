// Types related to the server side API.

export enum EntryType {
  ALBUM = 'album',
  PHOTO = 'photo',
  VIDEO = 'video',
}

export enum StereoMode {
  SIDE_BY_SIDE = 'sbs',
  GOOGLE_PHOTO = 'gphoto',
}

export interface AlbumEntry {
  name: string;
  type: EntryType;
  stereo?: StereoMode;
}

// Response type for GET /api/album?path=... send via JSON by the server.
export interface AlbumResponse {
  path: string;
  entries: AlbumEntry[];
}

// Types exclusively used in the client.

export interface SelectedMedia {
  album: AlbumResponse;
  index: number;
}
