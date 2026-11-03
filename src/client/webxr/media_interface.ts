import * as types from '../modules/client_types';

export interface PhotoState { }

export interface VideoState {
  duration: number;   // seconds, NaN if unknown
  currentTime: number; // seconds
  paused: boolean;
  muted: boolean;
}

export interface SplatState { }

export type MediaCallback = () => void;

/**
 * MediaInterface is the single source of truth for the current media state.
 * It is owned by the WebXRSession and consumed by the MediaRenderer and the
 * UserInterface. The current media's behavior is controlled through this
 * interface; the user interface reads from it and emits actions back.
 */
export class MediaInterface {
  // Current media, filename, and type.
  private media?: types.SelectedMedia;

  // Media state. Unset when the current media is of different type.
  private photoState: PhotoState | undefined = undefined;
  private videoState: VideoState | undefined = undefined;
  private splatState: SplatState | undefined = undefined;

  // Change callbacks.
  private mediaChangedCallbacks: MediaCallback[] = [];
  private videoPauseCallbacks: MediaCallback[] = [];
  private videoMuteCallbacks: MediaCallback[] = [];

  getMedia(): types.SelectedMedia | undefined {
    return this.media;
  }

  setMedia(media: types.SelectedMedia) {
    this.media = media;
    this.changeMedia(0);
  }

  getMediaEntry(): types.AlbumEntry | undefined {
    if (!this.media) return undefined;
    return this.media.album.entries[this.media.index];
  }

  getFilename(): string {
    return this.getMediaEntry()?.name ?? '';
  }

  getMediaType(): types.MediaType {
    return this.getMediaEntry()?.media ?? types.MediaType.UNKNOWN;
  }

  getPhotoState(): PhotoState | undefined {
    return this.photoState;
  }

  setPhotoState(state?: PhotoState): void {
    this.photoState = state;
    this.videoState = undefined;
    this.splatState = undefined;
  }

  getVideoState(): VideoState | undefined {
    return this.videoState;
  }

  setVideoState(state?: VideoState): void {
    this.photoState = undefined;
    this.videoState = state;
    this.splatState = undefined;
  }

  getSplatState(): SplatState | undefined {
    return this.splatState;
  }

  setSplatState(state?: SplatState): void {
    this.photoState = undefined;
    this.videoState = undefined;
    this.splatState = state;
  }

  changeMedia(delta: number) {
    if (!this.media) return;

    // Advance the media index.
    const album = this.media.album;
    const count = album.entries.length;
    this.media.index = (this.media.index + delta + count) % count;

    // Emit media changed callbacks.
    for (const cb of this.mediaChangedCallbacks) cb();
  }

  setVideoPause(paused: boolean) {
    if (!this.videoState) return;
    this.videoState.paused = paused;
    for (const cb of this.videoPauseCallbacks) cb();
  }

  toggleVideoPause() {
    if (!this.videoState) return;
    this.setVideoPause(!this.videoState.paused);
  }

  setVideoMute(muted: boolean) {
    if (!this.videoState) return;
    this.videoState.muted = muted;
    for (const cb of this.videoMuteCallbacks) cb();
  }

  toggleVideoMute() {
    if (!this.videoState) return;
    this.setVideoMute(!this.videoState.muted);
  }

  addMediaChangedCallback(cb: MediaCallback) {
    this.mediaChangedCallbacks.push(cb);
  }

  addVideoPauseCallback(cb: MediaCallback) {
    this.videoPauseCallbacks.push(cb);
  }

  addVideoMuteCallback(cb: MediaCallback) {
    this.videoMuteCallbacks.push(cb);
  }
}
