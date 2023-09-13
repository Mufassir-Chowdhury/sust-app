export class NavItem {

    constructor(name: string, url: string, icon: string) {
        this.name = name;
        this.url = url;
        this.icon = icon;
    }
    name: string;
    url: string;
    icon: string;
}
export class Group {
    constructor(title: string, pages: NavItem[]) {
        this.title = title;
        this.pages = pages;
    }
    title: string;
    pages: NavItem[];
}

export class ListTile{
    name: String;
    id: String;
    constructor(name: String, id: String){
        this.name = name;
        this.id = id;
    }
}