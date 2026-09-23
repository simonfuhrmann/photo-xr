// Error type thrown by all functions in this file.
export class HttpError extends Error {
  public status: number;
  public body: string;

  constructor(status: number, body: string) {
    super();
    this.status = status;
    this.body = body;
  }

  override toString(): string {
    return `HTTP ${this.status}: ${this.body}`
  }
}

// Sends a GET request to the given URL and returns JSON.
async function getJson(url: string): Promise<any> {
  const response = await fetch(url, { method: 'GET' });
  if (!response.ok) {
    const body = await response.text();
    throw new HttpError(response.status, body);
  }
  return (await response.json());
}

// Gets the album information for a path.
export async function getAlbum(path: string): Promise<any> {
  return getJson(`/api/album?path=${path}`);
}
