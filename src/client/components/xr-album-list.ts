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

  override connectedCallback(): void {
    super.connectedCallback();
    this.onRequest();
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
    if (!this.apiData) return nothing;
    const icon = getIconForFolder(this.isLoading, this.open);
    const isSelected = this.open && this.selected === '';
    const onClick = () => {
      this.selected = '';
      const data = this.apiData;
      const eventOptions = { detail: data, bubbles: true, composed: true };
      this.dispatchEvent(new CustomEvent('selected', eventOptions));
    };
    return html`
      <div id="entry" @click=${onClick} ?selected=${isSelected}>
        <oxy-icon ?spin=${this.isLoading} icon="${icon}"></oxy-icon>
        <div>${this.name}</div>
      </div>
      <div id="albums">${this.renderAlbums()}</div>
    `;
  }

  private renderAlbums() {
    if (!this.apiData || !this.open) return nothing;
    if (!this.open) return nothing;
    return this.apiData.entries.map(this.renderAlbum.bind(this));
  }

  private renderAlbum(entry: types.AlbumEntry) {
    if (entry.type !== types.EntryType.ALBUM) return nothing;
    const albumPath = `${this.path}/${entry.name}`;
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
    }).catch((err: api.HttpError) => {
      this.apiError = err;
    }).finally(() => {
      clearTimeout(timeout);
      this.isLoading = false;
    });
  }
}

function getIconForFolder(isLoading: boolean, isOpen: boolean) {
  if (isLoading) return 'image:autorenew';
  return isOpen ? 'icons:folder' : 'icons:folder-open';
}
