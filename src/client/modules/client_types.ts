// Types related to the server side API.

export enum MediaType {
  UNKNOWN = 'UNKNOWN',
  ALBUM = 'ALBUM',
  IMAGE_SBS = 'IMAGE_SBS',
  IMAGE_GPHOTO = 'IMAGE_GPHOTO',
  VIDEO_SBS = 'VIDEO_SBS',
  GEOMETRY_SPLAT = 'GEOMETRY_SPLAT',
}

export interface AlbumEntry {
  name: string;
  media: MediaType;
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
