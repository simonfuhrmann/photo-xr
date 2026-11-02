// Types related to the server side API.

export enum MediaType {
  UNKNOWN = 'UNKNOWN',
  ALBUM = 'ALBUM',
  IMAGE = 'IMAGE',
  VIDEO = 'VIDEO',
  SPLAT = 'SPLAT',
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
