export enum EntryType {
  ALBUM = 'album',
  PHOTO = 'photo',
}

export interface AlbumEntry {
  name: string;
  type: EntryType;
  stereo?: string;  // "sbs" or "gphoto"
}

export interface AlbumResponse {
  path: string;
  entries: AlbumEntry[];
}
