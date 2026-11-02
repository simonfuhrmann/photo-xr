import { html, css, LitElement, nothing } from 'lit';
import { customElement, property, state } from 'lit/decorators.js';

import 'oxygen-mdc/oxy-icon';
import 'oxygen-mdc/oxy-icons-base'
import 'oxygen-mdc/oxy-icons-image'

import * as api from '../modules/server_api';
import * as types from '../modules/client_types';
import * as stringUtils from '../modules/string_utils';

@customElement('xr-album-list')
export class XrAlbumList extends LitElement {
  static styles = css`
    :host {
      display: flex;
      flex-direction: column;
    }
    #entry {
      display: flex;
      flex-direction: row;
      align-items: center;
      cursor: pointer;
      padding: 2px 8px;
      margin: 1px;
      border-radius: 3px;
      user-select: none;
    }
    #entry[selected] {
      background-color: rgba(255, 255, 255, 0.1);
    }
    #entry oxy-icon {
      margin-right: 4px;
    }
    #entry oxy-icon[spin] {
      animation: spin 2s linear infinite;
    }
    #albums {
      display: flex;
      flex-direction: column;
      margin-left: 16px;
    }

    @keyframes spin {
      from { transform: rotate(0deg); }
      to { transform: rotate(360deg); }
    }
  `;

  @property({ type: String }) path = '';
  @property({ type: String }) name = '';
  @property({ type: Boolean }) open = false;
  @state() private selected = '';
  @state() private isLoading = false;
  @state() private apiError?: api.HttpError;
  @state() private apiData?: types.AlbumResponse;

  override updated(changedProperties: Map<string, any>) {
    if (changedProperties.has('path')) {
      this.apiData = undefined;
    }
    if (changedProperties.has('open') && this.open && !this.apiData) {
      this.onRequest();
    }
  }

  override render() {
    return [
      this.renderError(),
      this.renderData(),
    ];
  }

  private renderError() {
    if (!this.apiError) return nothing;
    return html`<div class="error">${this.apiError.toString()}</div>`;
  }

  private renderData() {
    const icon = getIconForFolder(this.isLoading, this.open);
    const isSelected = this.open && this.selected === '';
    const onClick = () => {
      this.selected = '';
      this.open = true;
      this.maybeDispatchEvent();
    };
    return html`
      <div id="entry" @click=${onClick} ?selected=${isSelected}>
        <oxy-icon ?spin=${this.isLoading} icon="${icon}"></oxy-icon>
        <div>${this.name}</div>
      </div>
      ${this.renderAlbums()}
    `;
  }

  private renderAlbums() {
    if (!this.open || !this.apiData) return nothing;
    return html`
      <div id="albums">
        ${this.apiData.entries.map(this.renderAlbum.bind(this))}
      </div>
    `;
  }

  private renderAlbum(entry: types.AlbumEntry) {
    if (entry.media !== types.MediaType.ALBUM) return nothing;
    const albumPath = stringUtils.joinPaths(this.path, entry.name);
    const isOpen = this.selected === entry.name;
    const onClick = () => {
      this.selected = entry.name;
    }
    return html`
      <xr-album-list
          path="${albumPath}" 
          name="${entry.name}/"
          ?open=${isOpen}
          @click=${onClick}>
      </xr-album-list>
    `;
  }

  private onRequest() {
    // Set a timeout after which to show a loading spinner.
    // Clear the timeout when the request finishes earlier.
    const timeout = setTimeout(() => {
      this.isLoading = true;
    }, 250);

    this.apiData = undefined
    api.getAlbum(this.path).then((data) => {
      this.apiData = data;
      this.maybeDispatchEvent();
    }).catch((err: api.HttpError) => {
      this.apiError = err;
    }).finally(() => {
      clearTimeout(timeout);
      this.isLoading = false;
    });
  }

  private maybeDispatchEvent() {
    if (!this.apiData) return;
    const data = this.apiData;
    const eventOptions = { detail: data, bubbles: true, composed: true };
    this.dispatchEvent(new CustomEvent('selected', eventOptions));
  }
}

function getIconForFolder(isLoading: boolean, isOpen: boolean) {
  if (isLoading) return 'image:autorenew';
  return isOpen ? 'icons:folder' : 'icons:folder-open';
}
